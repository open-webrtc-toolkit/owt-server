
#include "plugin.h"
#include <iostream>
#include <string.h>
#include <functional>
#include <vector>
#include <utility>
#include <map>
#include <inference_engine.hpp>
#include <opencv2/opencv.hpp>


class threading_class
{
    public:
        threading_class();
        void make_thread();
    private:
        void threading_func(); /* { std::cout << "Hello\n"; }*/
};

struct recog_result{
    cv::Rect rectangle;
    float value;
    std::string name;
    std::string emotion;
};
