{
    'includes': [
        '../../../build/common.gypi',
    ],
    'targets': [
        {
            'target_name': 'dump_syms',
            'type': 'executable',
            'sources': [
                'dump_syms.cc',
            ],
            'dependencies': [
                '../../../common/windows/common_windows.gyp:release/lib/common_windows_lib',
            ],
        },
    ],
}
