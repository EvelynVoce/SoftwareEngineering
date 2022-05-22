#ifndef PARSEGPX_H_201220
#define PARSEGPX_H_201220

#include <string>
#include <vector>
#include <fstream>
#include <iomanip>
#include <algorithm>

#include "points.h"
#include "xml/parser.h"
namespace GPX
{

  // Extra functions declared that are used in both parseRoute and parseTrack
  XML::Element convert_source(std::string source, bool isFileName);
  void check_source_element_is_valid(XML::Element element, std::string sub_element_to_find);
  void validate_sub_element(XML::Element element, std::string sub_element_to_find, bool is_track);
  std::string get_name(XML::Element element);
  std::string format_name(std::string name);
  GPS::Position get_next_position(XML::Element element);

  //Extra functions that are only used in parseTrack
  tm get_time(XML::Element element);
  std::vector<GPS::TrackPoint> get_track_data(XML::Element element_track, std::vector<GPS::TrackPoint> result);

  /*  Parse GPX data containing a route.
   *  The source data can be provided as a string, or from a file; which one is determined by the bool parameter.
   */
  std::vector<GPS::RoutePoint> parseRoute(std::string source, bool isFileName);


  /*  Parse GPX data containing a track.
   *  The source data can be provided as a string, or from a file; which one is determined by the bool parameter.
   */
  std::vector<GPS::TrackPoint> parseTrack(std::string source, bool isFileName);
}

#endif
