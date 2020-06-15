{
    'targets': [{
        'target_name': 'render_text',
        'dependencies': ["<!(node -p \"require('node-addon-api').gyp\")"],
        'conditions': [
            ['OS=="win"', {
                'msvs_settings': {
                    'VCCLCompilerTool': {
                        'RuntimeLibrary': 1,
                        'ExceptionHandling': 1,
                        'AdditionalOptions': ['/std:c++17']
                    },
                    'VCLinkerTool': {
                        'AdditionalLibraryDirectories': [
                            '<(module_root_dir)/deps/windows-<(target_arch)/lib'
                        ]
                    }
                },
                'defines': ['_HAS_EXCEPTIONS=1'],
                'include_dirs': [
                    '<(module_root_dir)/deps/windows-<(target_arch)/include'
                ],
                'libraries': [
                    'SDL2.lib', 'SDL2_ttf.lib', 'SDL2_image.lib', 'freetype.lib', 'bz2.lib', 'libpng16.lib',
                    'Imm32.lib', 'Setupapi.lib', 'Version.lib', 'Winmm.lib'
                ]
            }],
            ['OS=="linux"', {
                'cflags_cc': ['-std=c++17', '-fexceptions'],
                'libraries': ['-lSDL2', '-lSDL2_ttf', '-lSDL2_image']
            }]
        ],
        'defines': ['NAPI_CPP_EXCEPTIONS', 'GUI'],
        'include_dirs': [
            "<!@(node -p \"require('node-addon-api').include\")",
        ],
        'sources': [
            'src/addon/main.cpp'
        ]
    }]
}
