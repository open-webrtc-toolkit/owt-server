#!/usr/bin/env python
# Copyright (C) <2-21> Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

"""
Prepare testing environment.
"""

import os
import shutil
import sys
import subprocess
import pathlib

TEST_PATH = pathlib.Path(__file__).resolve().parents[1]
CODE_PATH = TEST_PATH.parents[3]
JS_SAMPLE_PATH = CODE_PATH/'dist'/'apps'/'current_app'

# Files needed for testing.
file_list = ['owt.js', 'rest-sample.js']


def _copy_js_sdk_and_sample():
    for file_name in file_list:
        shutil.copy(JS_SAMPLE_PATH/'public' /
                    'scripts'/file_name, TEST_PATH/'js'/'deps')
    

def main(argv):
    _copy_js_sdk_and_sample()
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
