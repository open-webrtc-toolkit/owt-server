/*
 * Config.cpp
 *
 *  Created on: Nov 4, 2014
 *      Author: qzhang8
 */

#include "Config.h"
#include "VideoCompositor.h"
#include <sstream>
#include <boost/foreach.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

namespace mcu {

Config* Config::m_config = nullptr;

const char* VideoResString[] = {"cif", "vga", "hd_720p", "sif", "hvga", "r480x360", "qcif", "r192x144", "hd_1080p", "uhd_4k"};

inline void Config::signalConfigChanged()
{
	for( std::list<ConfigListner*>::iterator listnerIter = m_configListner.begin();
			listnerIter != m_configListner.end();
			listnerIter++) {
		(*listnerIter)->onConfigChanged();
	}
}

VideoLayout* Config::getVideoLayout()
{
	return m_videoLayout.get();
}

void Config::setVideoLayout(const std::string& layoutStr) {
	boost::property_tree::ptree pt;
	std::istringstream is (layoutStr);
	boost::property_tree::read_json (is, pt);
	VideoLayout layout;
	std::string size = pt.get<std::string> ("root.size");
	layout.rootsize = VideoResolutionType::vga;
	for(int i = 0; i < VideoResolutionType::total; i++) {
		if (!size.compare(VideoResString[i])) {
			layout.rootsize = (VideoResolutionType)i;
			break;
		}
	}

	BOOST_FOREACH(boost::property_tree::ptree::value_type &regionlist, pt.get_child("region"))
	{
		Region region;
		region.id = regionlist.second.get<std::string>("id");
		region.left = regionlist.second.get<float>("left");
		region.top = regionlist.second.get<float>("top");
		region.relativesize = regionlist.second.get<float>("relativesize");
		layout.regions.push_back(region);
	}

	setVideoLayout(layout);
}
void Config::setVideoLayout(VideoLayout& layout)
{
	m_videoLayout.reset(new VideoLayout(layout));
	signalConfigChanged();
}

void Config::registerListner(ConfigListner* listner) {
	m_configListner.push_back(listner);
}

void Config::unregisterListner(ConfigListner* listner) {
	m_configListner.remove(listner);
}

Config::Config() {
	m_videoLayout.reset(nullptr);
};

} /* namespace mcu */
