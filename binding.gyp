{
    'variables': {
        'win_ia32_include_dir': '<(module_root_dir)/deps/windows-ia32/include',
        'win_ia32_lib_dir': '<(module_root_dir)/deps/windows-ia32/lib',
    },
    'targets': [{
        'target_name': 'render_text',
        'defines': ['NAPI_CPP_EXCEPTIONS', 'GUI'],
        'dependencies': ["<!(node -p \"require('node-addon-api').gyp\")"],        
        'msvs_settings': {
            'VCCLCompilerTool': {
                'RuntimeLibrary': 1,
				'ExceptionHandling': 1,
                'AdditionalOptions': ['/std:c++17']
            }
        },
        'conditions': [ 
            ['OS=="win"', {
                'defines': [
  					'_HAS_EXCEPTIONS=1'
				],
                'libraries': [
                    'SDL2.lib',
                    'SDL2_ttf.lib',
                    'SDL2_image.lib',
                    'freetype.lib',
                    'bz2.lib',
                    'libpng16.lib',

                    'Imm32.lib',
                    'Setupapi.lib',
                    'Version.lib',
                    'Winmm.lib'
                ],
            }],  
            ['OS=="win" and target_arch=="ia32"', {
                'include_dirs': [
                    '<(win_ia32_include_dir)'
                ],
                'msvs_settings': {
                    'VCLinkerTool': {
                        'AdditionalLibraryDirectories': [
                            '<(win_ia32_lib_dir)'
                        ]
                    }
                }
            },
            ['OS=="linux"'], {
                'cflags_cc': ['-std=c++17', '-fexceptions'],
                'libraries': ['-lSDL2 -lSDL2_ttf -lSDL2_image']
            }]
        ],
        'include_dirs': [
            "<!@(node -p \"require('node-addon-api').include\")",  
        ],
        'sources': [
            'src/addon/main.cpp'
        ]
    }]
}
