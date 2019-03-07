{
    'includes': [
        '../../build/common.gypi'
    ],
    'targets': [
        {
            'target_name': 'build_all',
            'type': 'none',
            'dependencies': [
                './crash_generation/crash_generation.gyp:*',
                './handler/exception_handler.gyp:*',
                './sender/crash_report_sender.gyp:*',
            ]
        },
        {
            'target_name': 'common',
            'type': 'static_library',
            'include_dirs': [
                '<(DEPTH)',
            ],
            'direct_dependent_settings': {
                'include_dirs': [
                    '<(DEPTH)',
                ]
            },
            'sources': [
                '<(DEPTH)/common/windows/guid_string.cc',
                '<(DEPTH)/common/windows/guid_string.h',
                '<(DEPTH)/common/windows/http_upload.h',
                '<(DEPTH)/common/windows/string_utils.cc',
            ]
        }
    ]
}
