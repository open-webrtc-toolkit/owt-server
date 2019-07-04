//Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0
//

#include <stdio.h>
#include <getopt.h>
#include <unistd.h>

#include <opencv2/opencv.hpp>

#include "common.h"
#include "face_detection.h"

using namespace std;

static void usage(int argc, char *argv[]) {
    fprintf(stderr, "usage: %s\n", argv[0]);
    fprintf(stderr, "   -i  input file path\n");
    fprintf(stderr, "   -s  show output\n");
    fprintf(stderr, "   -h  help\n");

    fprintf(stderr, "example:\n");
    fprintf(stderr, "   %s  -i  someone.png\n", argv[0]);

    exit(1);
}

int main(int argc, char* argv[]) {
    string I_in_filename;
    bool I_show = false;

    // parse input parameters
    int c;

    while(1) {
        c = getopt_long(argc, argv, "i:h",
                NULL, NULL);
        if (c == -1)
            break;

        switch (c) {
            case 'i':
                I_in_filename = optarg;
                break;

            case 's':
                I_show = true;
                break;

            case 'h':
                usage(argc, argv);
                break;

            default:
                cout << "Invalid opt: " << c << endl;
                usage(argc, argv);
                break;
        }
    }

    if (I_in_filename.empty()) {
        cout << "Error, no input file" << endl;

        usage(argc, argv);
        return 1;
    }

    cv::Mat image = cv::imread(I_in_filename.c_str(), cv::IMREAD_COLOR);
    if (image.empty()) {
        cout << "Read input image error: " << I_in_filename << endl;
        return 1;
    }

    FaceDetection detection;
    detection.init();

    std::vector<FaceDetection::DetectedObject> result;
    detection.infer(image, result);

    for (auto &obj : result) {
        cv::rectangle(image, obj.rect, cv::Scalar(255, 255, 0), 2);

        char msg[128];
        snprintf(msg, 128, "conf: %.2f", obj.confidence);

        int font_face = cv::FONT_HERSHEY_COMPLEX_SMALL;
        double font_scale = 1;
        int thickness = 1;
        int baseline;
        cv::Size text_size = cv::getTextSize(msg, font_face, font_scale, thickness, &baseline);

        cv::Point origin;
        origin.y = obj.rect.y - 10;
        if (obj.rect.width > text_size.width)
            origin.x = obj.rect.x + (obj.rect.width - text_size.width) / 2;
        else
            origin.x = obj.rect.x;

        cv::putText(image,
                msg,
                origin,
                font_face,
                font_scale,
                cv::Scalar(255, 255, 0),
                thickness
                );
    }

    if (I_show) {
        cv::imshow("output", image);
        cv::waitKey(0);
    } else {
        char name[128];
        snprintf(name, 128, "output.jpg");
        cv::imwrite(name, image);
        printf("Dump %s\n", name);
    }

    return 0;
}
