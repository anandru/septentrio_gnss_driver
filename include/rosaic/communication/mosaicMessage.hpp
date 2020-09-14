// *****************************************************************************
//
// © Copyright 2020, Septentrio NV/SA.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//    1. Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//    2. Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//    3. Neither the name of the copyright holder nor the names of its
//       contributors may be used to endorse or promote products derived
//       from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE 
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE. 
//
// *****************************************************************************

// *****************************************************************************
//
// Boost Software License - Version 1.0 - August 17th, 2003
// 
// Permission is hereby granted, free of charge, to any person or organization
// obtaining a copy of the software and accompanying documentation covered by
// this license (the "Software") to use, reproduce, display, distribute,
// execute, and transmit the Software, and to prepare derivative works of the
// Software, and to permit third-parties to whom the Software is furnished to
// do so, all subject to the following:

// The copyright notices in the Software and this entire statement, including
// the above license grant, this restriction and the following disclaimer,
// must be included in all copies of the Software, in whole or in part, and
// all derivative works of the Software, unless such copies or derivative
// works are solely in the form of machine-executable object code generated by
// a source language processor.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
// SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
// FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//
// *****************************************************************************

//! 0x24 is ASCII for $ - 1st byte in each message
#define SBF_SYNC_BYTE_1 0x24 
//! 0x40 is ASCII for @ - 2nd byte to indicate SBF block
#define SBF_SYNC_BYTE_2 0x40 
//! 0x24 is ASCII for $ - 1st byte in each message
#define NMEA_SYNC_BYTE_1 0x24
//! 0x47 is ASCII for G - 2nd byte to indicate NMEA-type ASCII message
#define NMEA_SYNC_BYTE_2_1 0x47
//! 0x50 is ASCII for P - 2nd byte to indicate proprietary ASCII message
#define NMEA_SYNC_BYTE_2_2 0x50
//! 0x24 is ASCII for $ - 1st byte in each response from mosaic
#define RESPONSE_SYNC_BYTE_1 0x24
//! 0x52 is ASCII for R (for "Response") - 2nd byte in each response from mosaic
#define RESPONSE_SYNC_BYTE_2 0x52
//! 0x0D is ASCII for "Carriage Return", i.e. "Enter"
#define CARRIAGE_RETURN 0x0D
//! 0x0A is ASCII for "Line Feed", i.e. "New Line"
#define LINE_FEED 0x0A

// C++ libraries
#include <cstddef>
#include <sstream>
#include <map>
// Boost includes
#include <boost/tokenizer.hpp>
#include <boost/call_traits.hpp>
#include <boost/format.hpp>
#include <boost/math/constants/constants.hpp>
// ROS includes
#include <ros/ros.h>
#include <rosaic/Gpgga.h>
#include <sensor_msgs/NavSatFix.h>
// ROSaic includes
#include <rosaic/parsers/nmea_parsers/gpgga.hpp>
#include <rosaic/crc/crc.h> 
#include <rosaic/parsers/string_utilities.h>
#include <rosaic/parsers/nmea_sentence.hpp>
#include <rosaic/PVTCartesian.h>
#include <rosaic/PVTGeodetic.h>
#include <rosaic/PosCovGeodetic.h>
#include <rosaic/AttEuler.h>
#include <rosaic/AttCovEuler.h>


#ifndef MOSAIC_MESSAGE_HPP
#define MOSAIC_MESSAGE_HPP

/**
 * @file mosaicMessage.hpp
 * @date 20/08/20
 * @brief Defines a class that can deal with a buffer of size bytes_transferred that is handed over from async_read_some
 */
 
extern bool use_GNSS_time;
//! Since switch only works with int (yet NMEA message IDs are strings), we need enum.
//! Note drawbacks: No variable can have a name which is already in some enumeration, enums are not type safe etc..
enum NMEA_ID_Enum {evGPGGA, evNavSatFix, evPVTCartesian, evPVTGeodetic, evPosCovGeodetic, evAttEuler, evAttCovEuler};
//! Static keyword makes them visible only to the code of this particular .cpp file (and those that import it), which helps to avoid global namespace pollution.
//! One also receives "multiple definition of 'StringValues'" error without the static keyword.
static std::map<std::string, NMEA_ID_Enum> StringValues;
static void StringValues_Initialize()
{
	StringValues.insert(std::make_pair("$GPGGA", evGPGGA));
	StringValues.insert(std::make_pair("NavSatFix", evNavSatFix));
	StringValues.insert(std::make_pair("4006", evPVTCartesian));
	StringValues.insert(std::make_pair("4007", evPVTGeodetic));
	StringValues.insert(std::make_pair("5906", evPosCovGeodetic));
	StringValues.insert(std::make_pair("5938", evAttEuler));
	StringValues.insert(std::make_pair("5939", evAttCovEuler));
}
  
namespace io_comm_mosaic 
{
	/**
	 * @brief Calculates the timestamp, in the Unix Epoch time format, either using the TOW as transmitted with the SBF block, or using the current time
	 * @param[in] TOW Number of milliseconds that elapsed since the beginning of the current GPS week as transmitted by the SBF block
	 * @param[in] use_GNSS_time If true, the TOW as transmitted with the SBF block is used, otherwise the current time
	 * @return ros::Time object containing seconds and nanoseconds since last epoch
	 */
	ros::Time TimestampSBF(uint32_t TOW, bool use_GNSS);
	
	/**
	 * @class mosaicMessage
	 * @brief Can search buffer for messages, read/parse them, and so on
	 */
	class mosaicMessage
	{
	public:
			/**
			 * @brief Constructor of the mosaicMessage class
			 * 
			 * One can always provide a non-const value where a const one was expected. The const-ness of the argument just means the function promises not to change it..
			 * Recall: static_cast by the way can remove or add const-ness, no other C++ cast is capable of removing it (not even reinterpret_cast)
			 * @param[in] data Pointer to the buffer that is about to be analyzed
			 * @param[in] size Size of the buffer (as handed over by async_read_some)
			 */
			mosaicMessage(const uint8_t* data, std::size_t& size): data_(data), count_(size) {found_ = false; CRCcheck_ = false; segment_size_ = 0;}
			
			//! Determines whether data_ points to the SBF block with ID "ID", e.g. 5003
			bool IsMessage(const uint16_t ID);
			//! Determines whether data_ points to the NMEA message with ID "ID", e.g. "$GPGGA"
			bool IsMessage(std::string ID);
			//! Determines whether data_ currently points to an SBF block
			bool IsSBF();
			//! Determines whether data_ currently points to an NMEA message
			bool IsNMEA();
			//! Determines whether data_ currently points to an NMEA message
			bool IsResponse();
			//! ll
			std::size_t SegmentEnd();
			//! Returns the MessageID of the message where data_ is pointing at at the moment, SBF identifiers embellished with inverted commas, e.g. "5003"
			std::string MessageID(); 
			
			/**
			 * @brief Returns the count_ variable
			 * @return The variable count_
			 */
			uint32_t GetCount() {return count_;};
			/**
			 * @brief Searches the buffer for the beginning of the next message (NMEA or SBF)
			 * @return A pointer to the start of the next message.
			*/
			const uint8_t* Search();
			
			/**
			 * @brief Gets the length of the SBF block
			 *
			 * It determines the length from the header of the SBF block. The block length thus includes the header length.
			 * @return The length of the SBF block
			 */
			uint16_t BlockLength();
			
			/**
			 * @brief Gets the current position in the read buffer
			 * @return The current position of the read buffer
			 */
			const uint8_t* Pos();
			
			/**
			 * @brief Gets the end position in the read buffer
			 * @return A pointer pointing to just after the read buffer (matches search()'s pointer if no valid message is found)
			 */
			const uint8_t* End();
		  
			/**
			 * @brief Has an NMEA message or SBF block been found in the buffer?
			 * @returns True if a message with the correct header & length has been found.
			 */
			bool Found();
			
			
			/**
			 * @brief Goes to the start of the next message based on the calculated length of current message
			 */
			const uint8_t* Next();
			
			/**
			 * @brief Performs the CRC check (if SBF block) and populates message with the necessary content (mapped 1-to-1 if SBF, parsed if NMEA), works for pure ROS message mapping.
			 * @return True if read was successful, false otherwise
			 */
			template <typename T>
			bool Read(typename boost::call_traits<T>::reference message, std::string message_key, bool search = false); 
			
			/**
			 * @brief Whether or not a message has been found
			 */
			bool found_; 
			
		private:

			/**
			 * @brief The pointer to the buffer of messages
			 */
			const uint8_t* data_; 
			
			/**
			 * @brief The number of bytes in the buffer, decremented as the buffer is read (so it is the buffer's size in the beginning)
			 */
			uint32_t count_; 
			
			/**
			 * @brief Whether the CRC check as evaluated in the read() method was successful or not is stored here.
			 */
			bool CRCcheck_;
			
			//! Helps to determine size of response message / NMEA message / SBF block
			std::size_t segment_size_;
			
			//! Number of times the rosaic::PVTGeodetic message has been published
			static uint32_t read_count_pvtgeodetic_;
			
			//! Number of times the rosaic::PVTCartesian message has been published
			static uint32_t read_count_pvtcartesian_;
			
			//! Number of times the rosaic::PosCovGeodetic message has been published
			static uint32_t read_count_poscovgeodetic_;
			
			//! Number of times the rosaic::AttEuler message has been published
			static uint32_t read_count_atteuler_;
			
			//! Number of times the rosaic::AttCovEuler message has been published
			static uint32_t read_count_attcoveuler_;
			
			//! Number of times the sensor_msgs::NavSatFix message has been published
			static uint32_t read_count_navsatfix_;
			
			//! Number of times the nmea_msgs::Gpgga message has been published
			static uint32_t read_count_gpgga_;
			
			/**
			 * @brief Since NavSatFix etc. need PVTGeodetic, incoming PVTGeodetic blocks need to be stored
			 */
			PVTGeodetic last_pvtgeodetic_;
			
			/**
			 * @brief Since NavSatFix etc. need PosCovGeodetic, incoming PosCovGeodetic blocks need to be stored
			 */
			PosCovGeodetic last_poscovgeodetic_;
			
			/**
			 * @brief Since GPSFix etc. need AttEuler, incoming AttEuler blocks need to be stored
			 */
			AttEuler last_atteuler_;
			
			/**
			 * @brief Since GPSFix etc. need AttCovEuler, incoming AttCovEuler blocks need to be stored
			 */
			AttCovEuler last_attcoveuler_;
			
			/**
			 * @brief Callback function when reading PVTCartesian blocks
			 * @param[in] data The (packed and aligned) struct instance that we use to populate our ROS message rosaic::PVTCartesian
			 * @return A smart pointer to the ROS message rosaic::PVTCartesian just created
			 */
			rosaic::PVTCartesianPtr PVTCartesianCallback(PVTCartesian& data);
			
			/**
			 * @brief Callback function when reading PVTGeodetic blocks
			 * @param[in] data The (packed and aligned) struct instance that we use to populate our ROS message rosaic::PVTGeodetic
			 * @return A smart pointer to the ROS message rosaic::PVTGeodetic just created
			 */
			rosaic::PVTGeodeticPtr PVTGeodeticCallback(PVTGeodetic& data);
			
			/**
			 * @brief Callback function when reading PosCovGeodetic blocks
			 * @param[in] data The (packed and aligned) struct instance that we use to populate our ROS message rosaic::PosCovGeodetic
			 * @return A smart pointer to the ROS message rosaic::PosCovGeodetic just created
			 */
			rosaic::PosCovGeodeticPtr PosCovGeodeticCallback(PosCovGeodetic& data);
			
			/**
			 * @brief Callback function when reading AttEuler blocks
			 * @param[in] data The (packed and aligned) struct instance that we use to populate our ROS message rosaic::AttEuler
			 * @return A smart pointer to the ROS message rosaic::AttEuler just created
			 */
			rosaic::AttEulerPtr AttEulerCallback(AttEuler& data);
			
			/**
			 * @brief Callback function when reading AttCovEuler blocks
			 * @param[in] data The (packed and aligned) struct instance that we use to populate our ROS message rosaic::AttCovEuler
			 * @return A smart pointer to the ROS message rosaic::AttCovEuler just created
			 */
			rosaic::AttCovEulerPtr AttCovEulerCallback(AttCovEuler& data);
			
			/**
			 * @brief "Callback" function when constructing NavSatFix messages
			 * @return A smart pointer to the ROS message sensor_msgs::NavSatFix just created
			 */
			sensor_msgs::NavSatFixPtr NavSatFixCallback();
			
	};
	
	
	/**
	 * Note that boost::call_traits<T>::reference is more robust than traditional T&.
	 * Note that this function also assigns the appropriate derived parser in case T is an NMEA message.
	 * Note that putting the default in the definition's argument list instead of the declaration's is an added extra that is not available for function templates, hence no search = false here.
	 * Finally note that it is bad practice (one gets undefined reference to .. error) to separate the definition of template functions into the source file and declarations into header file. 
	 * Also note that the SBF block header part of the SBF-echoing ROS messages have ID fields that only show the block number as found in the firmware (e.g. 4007 for PVTGeodetic), without the revision number.
	 */
	template <typename T>
	bool mosaicMessage::Read(typename boost::call_traits<T>::reference message, std::string message_key, bool search) 
	{
		//ROS_DEBUG("message key is %s", message_key.c_str());
		if (search) this->Search();
		if (!Found()) return false; 
		if (this->IsSBF())
		{
			// If the CRC check is unsuccessful, throw an error message.
			CRCcheck_ = CRCIsValid(data_);
			if (!CRCcheck_)
			{
				throw std::runtime_error("CRC Check returned False. Not a valid data block, perhaps noisy. Ignore..");
			}
		}
		switch(StringValues[message_key])
		{
			case evPVTCartesian: // Position and velocity in XYZ
			{	// The curly bracket here is crucial: Declarations inside a block remain inside, and will die at the end of the block. Otherwise variable overloading etc.
				rosaic::PVTCartesianPtr msg = boost::make_shared<rosaic::PVTCartesian>();
				PVTCartesian pvtcartesian;
				memcpy(&pvtcartesian, data_, sizeof(pvtcartesian));
				msg = PVTCartesianCallback(pvtcartesian);
				msg->ROS_Header.seq = read_count_pvtcartesian_;
				msg->ROS_Header.frame_id = frame_id;
				uint32_t TOW = *(reinterpret_cast<const uint32_t *>(data_ + 8));
				ros::Time time_obj;
				time_obj = TimestampSBF(TOW, use_GNSS_time);
				msg->ROS_Header.stamp.sec = time_obj.sec;
				msg->ROS_Header.stamp.nsec = time_obj.nsec;
				msg->Block_Header.ID = 4006;
				memcpy(&message, msg.get(), sizeof(*msg));
				++read_count_pvtcartesian_;
				break;
			}
			case evPVTGeodetic: // Position and velocity in geodetic coordinate frame (ENU frame)
			{
				rosaic::PVTGeodeticPtr msg = boost::make_shared<rosaic::PVTGeodetic>();
				memcpy(&last_pvtgeodetic_, data_, sizeof(last_pvtgeodetic_));
				msg = PVTGeodeticCallback(last_pvtgeodetic_);
				msg->ROS_Header.seq = read_count_pvtgeodetic_;
				msg->ROS_Header.frame_id = frame_id;
				uint32_t TOW = *(reinterpret_cast<const uint32_t *>(data_ + 8));
				ros::Time time_obj;
				time_obj = TimestampSBF(TOW, use_GNSS_time);
				msg->ROS_Header.stamp.sec = time_obj.sec;
				msg->ROS_Header.stamp.nsec = time_obj.nsec;
				msg->Block_Header.ID = 4007;
				memcpy(&message, msg.get(), sizeof(*msg));
				++read_count_pvtgeodetic_;
				break;
			}
				
			case evPosCovGeodetic:
			{
				rosaic::PosCovGeodeticPtr msg = boost::make_shared<rosaic::PosCovGeodetic>();
				memcpy(&last_poscovgeodetic_, data_, sizeof(last_poscovgeodetic_));
				msg = PosCovGeodeticCallback(last_poscovgeodetic_);
				msg->ROS_Header.seq = read_count_poscovgeodetic_;
				msg->ROS_Header.frame_id = frame_id;
				uint32_t TOW = *(reinterpret_cast<const uint32_t *>(data_ + 8));
				ros::Time time_obj;
				time_obj = TimestampSBF(TOW, use_GNSS_time);
				msg->ROS_Header.stamp.sec = time_obj.sec;
				msg->ROS_Header.stamp.nsec = time_obj.nsec;
				msg->Block_Header.ID = 5906;
				memcpy(&message, msg.get(), sizeof(*msg));
				++read_count_poscovgeodetic_;
				break;
			}
			case evAttEuler:
			{
				rosaic::AttEulerPtr msg = boost::make_shared<rosaic::AttEuler>();
				memcpy(&last_atteuler_, data_, sizeof(last_atteuler_));
				msg = AttEulerCallback(last_atteuler_);
				msg->ROS_Header.seq = read_count_atteuler_;
				msg->ROS_Header.frame_id = frame_id;
				uint32_t TOW = *(reinterpret_cast<const uint32_t *>(data_ + 8));
				ros::Time time_obj;
				time_obj = TimestampSBF(TOW, use_GNSS_time);
				msg->ROS_Header.stamp.sec = time_obj.sec;
				msg->ROS_Header.stamp.nsec = time_obj.nsec;
				msg->Block_Header.ID = 5938;
				memcpy(&message, msg.get(), sizeof(*msg));
				++read_count_atteuler_;
				break;
			}
			case evAttCovEuler:
			{
				rosaic::AttCovEulerPtr msg = boost::make_shared<rosaic::AttCovEuler>();
				memcpy(&last_attcoveuler_, data_, sizeof(last_attcoveuler_));
				msg = AttCovEulerCallback(last_attcoveuler_);
				msg->ROS_Header.seq = read_count_attcoveuler_;
				msg->ROS_Header.frame_id = frame_id;
				uint32_t TOW = *(reinterpret_cast<const uint32_t *>(data_ + 8));
				ros::Time time_obj;
				time_obj = TimestampSBF(TOW, use_GNSS_time);
				msg->ROS_Header.stamp.sec = time_obj.sec;
				msg->ROS_Header.stamp.nsec = time_obj.nsec;
				msg->Block_Header.ID = 5939;
				memcpy(&message, msg.get(), sizeof(*msg));
				++read_count_attcoveuler_;
				break;
			}
			case evGPGGA:
			{
				boost::char_separator<char> sep("*");
				typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
				std::size_t end_point = std::min(static_cast<std::size_t>(this->End() - data_), static_cast<std::size_t>(82));
				std::string block_in_string(reinterpret_cast<const char*>(data_), end_point);
				tokenizer tokens(block_in_string, sep);
				
				std::string id = this->MessageID();
				std::string one_message = *tokens.begin();
				boost::char_separator<char> sep_2(",", "", boost::keep_empty_tokens);
				tokenizer tokens_2(one_message, sep_2);
				std::vector<std::string> body;
				for (tokenizer::iterator tok_iter = tokens_2.begin(); tok_iter != tokens_2.end(); ++tok_iter) // perhaps str.erase from <string.h> would be faster, but i would not know what exactly to do..
				{
					body.push_back(*tok_iter);
				}
				// Create NmeaSentence struct to pass to GpggaParser::ParseASCII
				rosaic_driver::NMEASentence gga_message(id, body);
				rosaic::GpggaPtr gpgga_ros_message_ptr;
				rosaic_driver::GpggaParser parser_obj;
				try
				{
					gpgga_ros_message_ptr = parser_obj.ParseASCII(gga_message);
				}
				catch (rosaic_driver::ParseException& e)
				{
					throw std::runtime_error(e.what());
				}
				gpgga_ros_message_ptr->header.seq = read_count_gpgga_;
				//ROS_DEBUG("ID is %s, size of message is %lu and sizeof ptr is %lu", gpgga_ros_message_ptr->message_id.c_str(), sizeof(message), sizeof(*gpgga_ros_message_ptr));
				memcpy(&message, gpgga_ros_message_ptr.get(), sizeof(*gpgga_ros_message_ptr));
				break;
			}
			case evNavSatFix:
			{
				sensor_msgs::NavSatFixPtr msg = boost::make_shared<sensor_msgs::NavSatFix>();
				try
				{
					msg = NavSatFixCallback();
				}
				catch (std::runtime_error& e) 
				{
					throw std::runtime_error(e.what());
				}
				msg->header.seq = read_count_navsatfix_;
				msg->header.frame_id = frame_id;
				uint32_t TOW = last_pvtgeodetic_.TOW;
				ros::Time time_obj;
				time_obj = TimestampSBF(TOW, use_GNSS_time);
				msg->header.stamp.sec = time_obj.sec;
				msg->header.stamp.nsec = time_obj.nsec;
				memcpy(&message, msg.get(), sizeof(*msg));
				++read_count_navsatfix_;
				break;
			}
			// Many more to be implemented...
		}
		return true;
	}
}

#endif // for MOSAIC_MESSAGE_HPP


