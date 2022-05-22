#include <boost/test/unit_test.hpp>

#include "track.h"
#include "gridworld_track.h"
#include "geometry.h"

using namespace GPS;
using namespace GridWorld;

///////////////////////////////////////////////////////////////////////////////

BOOST_AUTO_TEST_SUITE( track_maxSpeed )

const metres horizontalGridUnit = 100000;
const metres verticalGridUnit = 0;
GridWorldModel gwNearEquator {Earth::Pontianak,horizontalGridUnit,verticalGridUnit};
const double percentageTolerance = 0.2;


// Typical input
BOOST_AUTO_TEST_CASE( multiple_points_long_time )
{
    const std::vector<TrackPoint> trackPoints = GridWorldTrack("A3400B3200C4000D", gwNearEquator).toTrackPoints();
    const Track track {trackPoints};
    const speed expectedMaxSpeed = horizontalGridUnit/3200;
    speed actualSpeed = track.maxSpeed();
    BOOST_CHECK_CLOSE(actualSpeed, expectedMaxSpeed, percentageTolerance);
}

// Typical input - A test to make sure speed is being returned not velocity. I.e, actualSpeed is always positive.
BOOST_AUTO_TEST_CASE( absolute_speed_not_velocity )
{
    const std::vector<TrackPoint> trackPoints = GridWorldTrack("A3800B3500C4000D", gwNearEquator).toTrackPoints();
    const Track track {trackPoints};
    const speed expectedMaxSpeed = horizontalGridUnit/3500;

    speed actualSpeed = track.maxSpeed();
    BOOST_CHECK_CLOSE(actualSpeed, expectedMaxSpeed, percentageTolerance);
}

// Typical input
BOOST_AUTO_TEST_CASE( not_always_smallest_time )
{
    /* This checks that maxSpeed isn't always just picking the smallest time.
    Here we see that it takes 50 seconds to get from O to M
    but 60 seconds to get from L to O which would mean the speed is quicker in this case as it is a larger distance*/

    const std::vector<TrackPoint> trackPoints = GridWorldTrack("K300L60O50M", gwNearEquator).toTrackPoints();
    const Track track {trackPoints};
    const speed expectedMaxSpeed = 3*horizontalGridUnit/60;

    speed actualSpeed = track.maxSpeed();
    BOOST_CHECK_CLOSE(actualSpeed, expectedMaxSpeed, percentageTolerance);
}

// Typical input - Ensures maxSpeed is calculating distance correct when latitude and logitude both change.
BOOST_AUTO_TEST_CASE( move_latitude_and_longitude )
{
    const std::vector<TrackPoint> trackPoints = GridWorldTrack("A600Y", gwNearEquator).toTrackPoints();
    const Track track {trackPoints};
    metres distance = pythagoras(4*horizontalGridUnit,4*horizontalGridUnit);
    const speed expectedMaxSpeed = distance/600;

    speed actualSpeed = track.maxSpeed();
    BOOST_CHECK_CLOSE(actualSpeed, expectedMaxSpeed, percentageTolerance);
}


// Error case - When a time between two consecutive points is specified as 0 a specific domain_error should be thrown
BOOST_AUTO_TEST_CASE( zero_duration )
{
    const std::vector<TrackPoint> trackPoints = GridWorldTrack("A0B", gwNearEquator).toTrackPoints();
    const Track track {trackPoints};

    BOOST_REQUIRE_THROW(track.maxSpeed(), std::domain_error);
    try
    {
        track.maxSpeed();
    }
    catch (const std::domain_error & e)
    {
        BOOST_CHECK_EQUAL( e.what() , "Cannot compute speed over a zero duration.");
    }
}

// The test below is supposed to test that negative duration causes a domain error but it returns 0 and fails instead
// Error case - negative time between two successive points
BOOST_AUTO_TEST_CASE( negative_duration )
{
    const Position pos1 = Position(30,65);
    const Position pos2 = Position(40,75);

    const std::tm date_time = {.tm_hour = 6};
    const std::tm date_time2 = { .tm_hour = 5};

    const std::vector<TrackPoint> trackPoints = { { pos1, "P0", date_time},
                                                  { pos2, "P1", date_time2}
                                                };
    const Track track {trackPoints};

    BOOST_REQUIRE_THROW(track.maxSpeed(), std::domain_error);
    try
    {
        track.maxSpeed();
    }
    catch (const std::domain_error & e)
    {
        BOOST_CHECK_EQUAL( e.what() , "Cannot compute speed over a negative duration.");
    }
}


/* Edge case - A test to see ensure that the maxSpeed is calculated correctly if the maxSpeed is the
 first speed that can be calculated from the track */
BOOST_AUTO_TEST_CASE( first_speed_fastest )
{
    const std::vector<TrackPoint> trackPoints = GridWorldTrack("A20B30C40D50E", gwNearEquator).toTrackPoints();
    const Track track {trackPoints};
    const speed expectedMaxSpeed = horizontalGridUnit/20;

    speed actualSpeed = track.maxSpeed();
    BOOST_CHECK_CLOSE(actualSpeed, expectedMaxSpeed, percentageTolerance);
}

/* Edge case - A test to see ensure that the maxSpeed is calculated correctly if the maxSpeed is the
 last speed that can be calculated from the track */
BOOST_AUTO_TEST_CASE( last_speed_fastest )
{
    const std::vector<TrackPoint> trackPoints = GridWorldTrack("A60B50C40D30E", gwNearEquator).toTrackPoints();
    const Track track {trackPoints};
    const speed expectedMaxSpeed = horizontalGridUnit/30;

    speed actualSpeed = track.maxSpeed();
    BOOST_CHECK_CLOSE(actualSpeed, expectedMaxSpeed, percentageTolerance);
}

// Edge case - zero_distance (No movement should return 0, even if the time taken to travel there is specified)
BOOST_AUTO_TEST_CASE( time_but_zero_movement )
{
    const std::vector<TrackPoint> trackPoints = GridWorldTrack("L30L", gwNearEquator).toTrackPoints();
    const Track track {trackPoints};
    const speed expectedMaxSpeed = 0;

    speed actualSpeed = track.maxSpeed();
    BOOST_CHECK_EQUAL(actualSpeed, expectedMaxSpeed);
}


// Boundary case - A track with just one point will return 0 as the expected maxSpeed
BOOST_AUTO_TEST_CASE( largest_invalid_track )
{
    const std::vector<TrackPoint> trackPoints = GridWorldTrack("A", gwNearEquator).toTrackPoints();
    const Track track {trackPoints};
    const speed expectedMaxSpeed = 0;

    speed actualSpeed = track.maxSpeed();
    BOOST_CHECK_EQUAL(actualSpeed, expectedMaxSpeed);
}

/* Boundary case - A track with two points and a given time to travel between
 those two points will return a maxSpeed that will not always be 0 */
BOOST_AUTO_TEST_CASE( smallest_valid_track )
{
    const std::vector<TrackPoint> trackPoints = GridWorldTrack("A10B", gwNearEquator).toTrackPoints();
    const Track track {trackPoints};
    const speed expectedMaxSpeed = horizontalGridUnit/10;;

    speed actualSpeed = track.maxSpeed();
    BOOST_CHECK_CLOSE(actualSpeed, expectedMaxSpeed, percentageTolerance);
}

BOOST_AUTO_TEST_SUITE_END()

///////////////////////////////////////////////////////////////////////////////
