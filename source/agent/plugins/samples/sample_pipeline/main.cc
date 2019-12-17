#include <iostream>
#include "mypipeline.h"

int main() {
    MyPipeline pipeline;
    std::unordered_map<std::string, std::string> plugin_config_map = {
        {"inputwidth", "672"},
        {"inputheight", "374"},
        {"inputframerate", "25"},
        {"pipelinename", "dc51138a8284436f873418a21ba8cfa7"} };
    pipeline.PipelineInit(plugin_config_map);
    pipeline.LinkElements();

    std::cout << "my pipeline initiate succeeds " <<  std::endl;
 
    return 1;
}
