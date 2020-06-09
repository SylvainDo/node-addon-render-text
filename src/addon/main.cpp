#include <filesystem>
#include <memory>

#include <napi.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>

namespace sdlw {
    struct Error : std::runtime_error {
        Error() :
            std::runtime_error{SDL_GetError()}
        {}
    };

    struct Init {
        Init() {
            if (TTF_Init() != 0)
                throw Error{};
        }

        ~Init() {
            TTF_Quit();
        }
    };

    struct Font {
        std::unique_ptr<TTF_Font, void(*)(TTF_Font*)> ptr{nullptr, TTF_CloseFont};

        Font(const std::filesystem::path& path, int size) {
            ptr.reset(TTF_OpenFont(path.string().c_str(), size));

            if (!ptr)
                throw Error{};
        }

        TTF_Font* get() const {
            return ptr.get();
        }

        void set_outline(int outline) {
            TTF_SetFontOutline(get(), outline);
        }
    };

    struct Surface {
        std::unique_ptr<SDL_Surface, void(*)(SDL_Surface*)> ptr{nullptr, SDL_FreeSurface};

        Surface(const Font& font, const std::string& text, const SDL_Color& color) {
            ptr.reset(TTF_RenderUTF8_Blended(font.ptr.get(), text.c_str(), color));

            if (!ptr)
                throw Error{};
        }

        Surface(void* pixels, int width, int height, int depth, int pitch, SDL_PixelFormatEnum format) {
            ptr.reset(SDL_CreateRGBSurfaceWithFormatFrom(pixels, width, height, depth, pitch, format));

            if (!ptr)
                throw Error{};
        }

        SDL_Surface* get() const {
            return ptr.get();
        }

        void set_blendmode(SDL_BlendMode blendmode) {
            SDL_SetSurfaceBlendMode(get(), blendmode);
        }

        void set_format(SDL_PixelFormatEnum format) {
            ptr.reset(SDL_ConvertSurfaceFormat(get(), format, 0));
        }

        static void blit(sdlw::Surface& src, sdlw::Surface& dst, SDL_Rect& dst_rect) {
            SDL_BlitSurface(src.get(), nullptr, dst.get(), &dst_rect);
        }

        void save_png(const std::filesystem::path& path) {
            if (IMG_SavePNG(get(), path.string().c_str()) != 0)
                throw Error{};
        }
    };
}

sdlw::Surface render_text_surface(sdlw::Font& font, sdlw::Font& outline_font, int outline_thickness,
    const std::string& text, const SDL_Color& color, const SDL_Color& outline_color) {

    sdlw::Surface bg_surface{outline_font, text, outline_color};
    sdlw::Surface fg_surface{font, text, color};

    fg_surface.set_blendmode(SDL_BLENDMODE_BLEND);
    SDL_Rect dst_rect{outline_thickness, outline_thickness, fg_surface.get()->w, fg_surface.get()->h};
    sdlw::Surface::blit(fg_surface, bg_surface, dst_rect);
    bg_surface.set_format(SDL_PIXELFORMAT_RGBA8888);

    return bg_surface;
}

SDL_Color to_color(const Napi::Value& value) {
    auto col = value.As<Napi::Object>();

    auto r = static_cast<Uint8>(col.Get("r").As<Napi::Number>().Uint32Value());
    auto g = static_cast<Uint8>(col.Get("g").As<Napi::Number>().Uint32Value());
    auto b = static_cast<Uint8>(col.Get("b").As<Napi::Number>().Uint32Value());

    return {r, g, b, 255};
}

Napi::Value render_text(const Napi::CallbackInfo& info) {
    try {
        auto font_path = info[0].As<Napi::String>().Utf8Value();
        auto font_size = info[1].As<Napi::Number>().Int32Value();
        auto text = info[2].As<Napi::String>().Utf8Value();
        auto fill_color = to_color(info[3]);
        auto outline_color = to_color(info[4]);
        auto outline_thickness = info[5].As<Napi::Number>().FloatValue();

        sdlw::Init init;

        sdlw::Font font{font_path, font_size};
        sdlw::Font outline_font{font_path, font_size};
        outline_font.set_outline(outline_thickness);

        auto s = render_text_surface(font, outline_font, outline_thickness, text, fill_color, outline_color);
        auto s_obj = Napi::Object::New(info.Env());

        s_obj.Set("w", s.get()->w);
        s_obj.Set("h", s.get()->h);
        s_obj.Set("data", Napi::Buffer<std::uint8_t>::Copy(info.Env(), static_cast<uint8_t*>(s.get()->pixels), s.get()->pitch * s.get()->h));

        return s_obj;
    }
    catch (const std::exception& e) {
        throw Napi::Error::New(info.Env(), e.what());
    }

    return info.Env().Undefined();
}

Napi::Value render_texts(const Napi::CallbackInfo& info) {
    try {
        auto font_path = info[0].As<Napi::String>().Utf8Value();
        auto font_size = info[1].As<Napi::Number>().Int32Value();
        auto texts = info[2].As<Napi::Array>();
        auto fill_color = to_color(info[3]);
        auto outline_color = to_color(info[4]);
        auto outline_thickness = info[5].As<Napi::Number>().FloatValue();

        sdlw::Init init;

        sdlw::Font font{font_path, font_size};
        sdlw::Font outline_font{font_path, font_size};
        outline_font.set_outline(outline_thickness);

        auto ret = Napi::Array::New(info.Env());

        for (std::size_t i{}; i < texts.Length(); ++i) {
            auto s = render_text_surface(font, outline_font, outline_thickness, texts.Get(i).As<Napi::String>().Utf8Value(), fill_color, outline_color);
            auto s_obj = Napi::Object::New(info.Env());

            s_obj.Set("w", s.get()->w);
            s_obj.Set("h", s.get()->h);
            s_obj.Set("data", Napi::Buffer<std::uint8_t>::Copy(info.Env(), static_cast<uint8_t*>(s.get()->pixels), s.get()->pitch * s.get()->h));

            ret.Set(i, s_obj);
        }

        return ret;
    }
    catch (const std::exception& e) {
        throw Napi::Error::New(info.Env(), e.what());
    }

    return info.Env().Undefined();
}

Napi::Value save_png(const Napi::CallbackInfo& info) {
    try {
        auto path = info[0].As<Napi::String>().Utf8Value();
        auto surface = info[1].As<Napi::Object>();

        auto w = surface.Get("w").As<Napi::Number>().Int32Value();
        auto h = surface.Get("h").As<Napi::Number>().Int32Value();
        auto data = surface.Get("data").As<Napi::Uint8Array>();

        sdlw::Surface s{data.Data(), w, h, 32, w * 4, SDL_PIXELFORMAT_RGBA8888};
        s.save_png(path);
    }
    catch (const std::exception& e) {
        throw Napi::Error::New(info.Env(), e.what());
    }

    return info.Env().Undefined();
}

Napi::Object build_module(Napi::Env env, Napi::Object exports) {
    exports.Set("renderText", Napi::Function::New(env, render_text));
    exports.Set("renderTexts", Napi::Function::New(env, render_texts));
    exports.Set("savePng", Napi::Function::New(env, save_png));

    return exports;
}

NODE_API_MODULE(NODE_GYP_MODULE_NAME, build_module)
