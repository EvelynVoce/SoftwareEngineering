#include "earth.h"
#include "parseNMEA.h"
#include <functional>
#include <numeric>
#include <sstream>
#include <assert.h>

namespace NMEA
{
  bool isSupportedSentenceFormat(std::string format)
  {
      // Valid formats defined in a vector so additonal formats could become valid in the future
      const std::vector<std::string> valid_formats = {"GLL", "GGA", "RMC"};
      const bool is_format_valid = std::find(valid_formats.begin(), valid_formats.end(), format) != valid_formats.end();
      return is_format_valid;
  }

  bool isWellFormedSentence(std::string candidateSentence)
  {
      const bool well_formed = true;
      //Checks that the candidateSentence is less than the minimum length required
      const int min_length_of_well_formed_sentence = 10;
      if(candidateSentence.length() < min_length_of_well_formed_sentence){
          return !well_formed;
      }

      //Checks candidateSentence only has 1 asterisk and dollar sign
      const size_t asterisk_count = std::count(candidateSentence.begin(), candidateSentence.end(), '*');
      const size_t dollar_count = std::count(candidateSentence.begin(), candidateSentence.end(), '$');
      const int max_number_of_asterisks = 1;
      const int max_number_of_dollar_signs = 1;
      if (asterisk_count > max_number_of_asterisks or dollar_count > max_number_of_dollar_signs){
          return !well_formed;
      }

      //Checks the prefix is $GP
      const int prefix_index = 3;
      const std::string prefix = candidateSentence.substr(0, prefix_index);
      if (prefix != "$GP"){
          return !well_formed;
      }

      //Checks candidateSentence has all 3 alpha characters required in the correct positons
      const int alph_chars_index = 3;
      const std::string alph_chars = candidateSentence.substr(prefix_index, alph_chars_index);
      const bool contains_non_alpha = std::find_if(alph_chars.begin(), alph_chars.end(), [](char c) { return !std::isalpha(c); }) != alph_chars.end();
      if (contains_non_alpha == true){
          return !well_formed;
      }

      //Checks candidateSentence has the asterisk in the correct position
      const int value_to_asterisk = 3;
      const int asterisk_index = candidateSentence.length() - value_to_asterisk;
      if (candidateSentence[asterisk_index] != '*'){
          return !well_formed;
      }

      //Checks checksum has 2 valid hex characters
      const int checksum_index = 2;
      const std::string checksum = candidateSentence.substr( candidateSentence.length() - checksum_index);
      const bool contains_invalid_hex_chars = std::find_if(checksum.begin(), checksum.end(), [](char c) { return !std::isxdigit(c); }) != checksum.end();
      if (contains_invalid_hex_chars){
          return !well_formed;
      }
      return well_formed;
  }

  bool hasCorrectChecksum(std::string sentence)
  {
      assert(isWellFormedSentence(sentence)); //Precondition: Sentence is a well formed sentence
      //Getting the checksum value and converting it into a decimal (base 16) value
      const int take_last_2_digits = 2;
      const int base = 16;
      const std::string checksum = sentence.substr( sentence.length() - take_last_2_digits ); //seperates the checksum from the sentence
      const int checksum_int = std::stoi (checksum, nullptr, base); //Converts the checksum from a string to a base 16 integer

      const int dollar_sign_index = 1;
      const int asterix_position = 3;
      const int initial_accumulator_val = 0x00;

      //Perform an xor reduction on every element in sentence between the $ and the *
      const int xor_reduction_val = std::accumulate (sentence.begin() + dollar_sign_index, sentence.end() - asterix_position, initial_accumulator_val, std::bit_xor<char>());
      const bool is_checksum_correct = checksum_int == xor_reduction_val;
      return is_checksum_correct;
  }

  SentenceData parseSentenceData(std::string sentence)
  {
      assert(isWellFormedSentence(sentence)); //Precondition: Sentence is a well formed sentence
      // Get format
      const int first_format_char = 3;
      const int last_format_char = 3;
      const std::string format = sentence.substr(first_format_char, last_format_char);

      //Get data fields
      const int postion_of_first_comma = sentence.find_first_of(',');
      const int value_to_get_one_after_first_comma = 1;
      const int value_to_get_four_before_first_comma = 4;
      const std::string data_fields = sentence.substr(postion_of_first_comma + value_to_get_one_after_first_comma
                    , sentence.length() - postion_of_first_comma - value_to_get_four_before_first_comma);

      //Split data fields
      std::stringstream ss(data_fields);
      std::vector<std::string> data_fields_split;
      while( ss.good() )
      {
          std::string substr;
          getline( ss, substr, ',' );
          data_fields_split.push_back( substr );
      }
      return {format,data_fields_split};
  }

  std::string getValueFromDataFields(SentenceData data, std::string to_find){
      const auto found_direction = std::find(data.dataFields.begin(), data.dataFields.end(), to_find);
      if (found_direction != data.dataFields.end())
      {
          const int index_direction_found = found_direction - data.dataFields.begin();
          // If N,S,W,E or M found at position 0, no value cannot come before it.
          const int minimum_index_a_direction_can_be = 1;
          if (index_direction_found < minimum_index_a_direction_can_be){
              throw std::invalid_argument("Index found at position 0. No corresponding value found");
          }
          const int get_from_direction_to_value = 1;
          return data.dataFields[index_direction_found - get_from_direction_to_value]; //Return the value found.

      } else{ //If the value that needed to be found was not found, throw an invalid argument
          throw std::invalid_argument("Value not found.");
      }
  }

  PositionDataWithDirection directionAndValueFinder(SentenceData data, std::string to_find_positive, std::string to_find_negative, char positive_char, char negative_char)
  {
      const bool positve_direction_found = std::find(data.dataFields.begin(), data.dataFields.end(), to_find_positive) != data.dataFields.end();
      if (positve_direction_found){
          const std::string value_found = getValueFromDataFields(data, to_find_positive);
          return {value_found, positive_char};
      } else{
          const std::string value_found = getValueFromDataFields(data, to_find_negative);
          return {value_found, negative_char};
      }
  }

  GPS::Position interpretSentenceData(SentenceData data)
  {
      if (!isSupportedSentenceFormat(data.format)){
          throw std::invalid_argument("Invalid data format.");
      }

      // DataField must include at least, latitude, direction1, longitude, direction2.
      const int minimum_size_of_valid_data_field = 4;
      if (int(data.dataFields.size()) < minimum_size_of_valid_data_field){
          throw std::invalid_argument("DataFields is not the required minimum size.");
      }

      // Only GGA formats have M's/ elevation values, otherwise elevation defaults to 0.
      std::string elevation = "0";
      std::string format_requires_elevation = "GGA";
      if (data.format == format_requires_elevation){
          elevation = getValueFromDataFields(data, "M");
      }
      const PositionDataWithDirection latitude = directionAndValueFinder(data, "N", "S", 'N', 'S');
      const PositionDataWithDirection longitude = directionAndValueFinder(data, "E", "W", 'E', 'W');
      return GPS::Position(latitude.value, latitude.direction, longitude.value, longitude.direction, elevation);
  }

  std::vector<GPS::Position> positionsFromLog(std::istream & log){
      std::string log_line;
      std::vector<GPS::Position> vector_of_positions = {};

      while(std::getline(log,log_line))
      {
          // parseSentenceData
          if (isWellFormedSentence(log_line)){
              SentenceData line_in_parsed_format = parseSentenceData(log_line);
              // Check Format
              bool isCorrectFormat = isSupportedSentenceFormat(line_in_parsed_format.format);

              // CheckSum Test
              bool isCorrectChecksum = hasCorrectChecksum(log_line);

              if (isCorrectChecksum and isCorrectFormat){
                  try {
                      GPS::Position sentenceData = interpretSentenceData(line_in_parsed_format);
                      vector_of_positions.push_back(sentenceData);
                  } catch (...) {
                  }
              }
          }
      }
      return {vector_of_positions};
      }
}
