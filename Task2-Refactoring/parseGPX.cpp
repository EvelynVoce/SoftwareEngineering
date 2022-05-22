#include "parseGPX.h"

const std::string track_point_literal = "trkpt";
const std::string latitude_literal = "lat";
const std::string longitude_literal = "lon";
const std::string time_literal = "time";

namespace GPX
{
  //Convert's the source from string to element
  XML::Element convert_source(std::string source, bool isFileName){
      if (isFileName) { // If the source is a file...
          std::ifstream file_stream(source);
          if (! file_stream.good()) throw std::invalid_argument("Error opening source file '" + source + "'.");
          source = ""; // Set the source to an empty string
          std::string line_data;
          while (file_stream.good()) { // read the file line by line and append it to the source string
              getline(file_stream, line_data);
              source += line_data;
          }
      }
      XML::Element element = XML::Parser(source).parseRootElement(); // Parses the source string into an XML element
      return element;
  }

  void check_source_element_is_valid(XML::Element element, std::string sub_element_to_find){
      const std::string required_format = "gpx";
      if (element.getName() != required_format) throw std::domain_error("Missing '" + required_format + "' element.");
      if (! element.containsSubElement(sub_element_to_find)) throw std::domain_error("Missing '" + sub_element_to_find + "' element.");
  }

  // Removes leading and trailing whitespace from the name
  std::string format_name(std::string name){
      const bool contains_non_white_space = std::find_if(name.begin(), name.end(), [](char c) {
          return !(c == ' '); }) != name.end();
      std::string formatted_name;
      if (contains_non_white_space) {
          const int first_non_white_space = name.find_first_not_of(' ');
          const int last_non_white_space = name.find_last_not_of(' ');
          const int position_after_last_white_space = last_non_white_space + 1;
          formatted_name = name.substr(first_non_white_space, position_after_last_white_space);
      }
      return formatted_name;
  }

  // If the element contains the name subelement, return the name as a string. Otherwise return an empty string
  std::string get_name(XML::Element element){
      std::string name = "";
      const std::string name_literal = "name";
      if (element.containsSubElement(name_literal)) {
          XML::Element ele_rtept_name = element.getSubElement(name_literal);
          name = ele_rtept_name.getLeafContent();
      }
      return name;
  }

  // Get's the time sub-element as a string and converts it to a time structure
  tm get_time(XML::Element element){
      XML::Element Element_time = element.getSubElement(time_literal);
      std::string time = Element_time.getLeafContent();

      // Converting the time from a string to tm
      tm time_structure;
      std::istringstream iss;
      iss.str(time);
      iss >> std::get_time(&time_structure,"%Y-%m-%dT%H:%M:%SZ");
      return time_structure;
  }

  // Returns the position of the element that is passed in
  GPS::Position get_next_position(XML::Element element){
      const std::string elevation_literal = "ele";
      std::string latitude = element.getAttribute(latitude_literal);
      std::string longitude = element.getAttribute(longitude_literal);
      XML::Element element_elevation = element.getSubElement(elevation_literal);
      std::string elevation = element_elevation.getLeafContent();
      return {latitude,longitude, elevation};
  }

  void validate_sub_element(XML::Element sub_element, std::string sub_element_to_find, bool is_track){
      if (! sub_element.containsSubElement(sub_element_to_find)) throw std::domain_error("Missing '" +  sub_element_to_find + "' element.");
      XML::Element sub_element_found = sub_element.getSubElement(sub_element_to_find);
      if (! sub_element_found.containsAttribute(latitude_literal)) throw std::domain_error("Missing 'lat' attribute.");
      if (! sub_element_found.containsAttribute(longitude_literal)) throw std::domain_error("Missing 'lon' attribute.");
      if (is_track){ // Track points must also include the time sub element but routes don't
          if (! sub_element_found.containsSubElement(time_literal)) throw std::domain_error("Missing 'time' element.");
      }
  }

  std::vector<GPS::RoutePoint> parseRoute(std::string source, bool isFileName)
  {
      // Validating element and route_element
      XML::Element element = convert_source(source, isFileName);
      const std::string route_literal = "rte";
      check_source_element_is_valid(element, route_literal);
      XML::Element route_element = element.getSubElement(route_literal);
      const std::string route_point_literal = "rtept";
      validate_sub_element(route_element, route_point_literal, false);

      // Pushing route points to the route_points_vector
      std::vector<GPS::RoutePoint> route_points_vector;
      const int amount_of_ele_rte_sub_elements = route_element.countSubElements(route_point_literal);
      for (int sub_element_index = 0; sub_element_index < amount_of_ele_rte_sub_elements; sub_element_index++) {
          XML::Element ele_rtept = route_element.getSubElement(route_point_literal, sub_element_index);
          GPS::Position postion_of_route = get_next_position(ele_rtept);
          std::string name = get_name(ele_rtept);
          route_points_vector.push_back({postion_of_route,format_name(name)});
      }
      return route_points_vector;
  }

  // pushing track points to the track_points_vector
  std::vector<GPS::TrackPoint> get_track_data(XML::Element element_track, std::vector<GPS::TrackPoint> track_points_vector){
      const int total_track_points = element_track.countSubElements(track_point_literal);
      for (int track_point_num = 0; track_point_num < total_track_points; track_point_num++) {
          XML::Element ele_track_point = element_track.getSubElement(track_point_literal, track_point_num);
          GPS::Position next_pos = get_next_position(ele_track_point);
          std::string name = get_name(ele_track_point);
          tm time = get_time(ele_track_point);
          track_points_vector.push_back({next_pos, format_name(name), time});
      }
      return track_points_vector;
  }

  // Returns a vector of parsed Track data
  std::vector<GPS::TrackPoint> parseTrack(std::string source, bool isFileName)
  {
      XML::Element element = convert_source(source, isFileName);
      const std::string track_literal = "trk";
      check_source_element_is_valid(element, track_literal);

      XML::Element track_element = element.getSubElement(track_literal);
      const std::string track_segment_literal = "trkseg";
      std::vector<GPS::TrackPoint> track_points_vector;
      if (track_element.containsSubElement(track_segment_literal)){
          const int total_track_segments = track_element.countSubElements(track_segment_literal);
          // push the track points from each segment to the track_points_vector
          for (int segNum = 0; segNum < total_track_segments; segNum++) {
              XML::Element specific_ele_trk = track_element.getSubElement(track_segment_literal,segNum);
              track_points_vector = get_track_data(specific_ele_trk, track_points_vector);
          }
      }
      else {
          validate_sub_element(track_element, track_point_literal, true); // Check that the track element is valid
          track_points_vector = get_track_data(track_element, track_points_vector);
      }
      return track_points_vector;
  }
}
