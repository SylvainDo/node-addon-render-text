#include <filesystem>
#include <memory>

#include <napi.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>

std::uint32_t pot(std::uint32_t v) {
    if (v != 0 ) {
        --v;
        v |= (v >> 1);
        v |= (v >> 2);
        v |= (v >> 4);
        v |= (v >> 8);
        v |= (v >> 16);
        ++v;
    }

    return v;
}

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

        Surface(int width, int height, int depth, SDL_PixelFormatEnum format) {
            ptr.reset(SDL_CreateRGBSurfaceWithFormat(0, width, height, depth, format));

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

        int width() const {
            return get()->w;
        }

        int height() const {
            return get()->h;
        }

        int pitch() const {
            return get()->pitch;
        }

        void* pixels() const {
            return get()->pixels;
        }
    };
}

sdlw::Surface render_text_surface(sdlw::Font& font, sdlw::Font& outline_font, int outline_thickness,
    const std::string& text, const SDL_Color& color, const SDL_Color& outline_color)
{
    sdlw::Surface bg_surface{outline_font, text, outline_color};
    sdlw::Surface fg_surface{font, text, color};

    fg_surface.set_blendmode(SDL_BLENDMODE_BLEND);
    SDL_Rect dst_rect{outline_thickness, outline_thickness, fg_surface.width(), fg_surface.height()};
    sdlw::Surface::blit(fg_surface, bg_surface, dst_rect);

    sdlw::Surface final_surface{static_cast<int>(pot(bg_surface.width())), static_cast<int>(pot(bg_surface.height())), 32, SDL_PIXELFORMAT_RGBA8888};
    SDL_Rect final_dst_rect{final_surface.width() / 2 - bg_surface.width() / 2,
        final_surface.height() / 2 - bg_surface.height() / 2,
        bg_surface.width(), bg_surface.height()};
    sdlw::Surface::blit(bg_surface, final_surface, final_dst_rect);

    return final_surface;
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

        s_obj.Set("w", s.width());
        s_obj.Set("h", s.height());
        s_obj.Set("data", Napi::Buffer<std::uint8_t>::Copy(info.Env(), static_cast<std::uint8_t*>(s.pixels()), s.pitch() * s.height()));

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

            s_obj.Set("w", s.width());
            s_obj.Set("h", s.height());
            s_obj.Set("data", Napi::Buffer<std::uint8_t>::Copy(info.Env(), static_cast<std::uint8_t*>(s.pixels()), s.pitch() * s.height()));

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
