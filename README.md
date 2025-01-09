# SLIVER Laser
 
General Idea: The SLIVER logger is intended to measure geodetic-grade GNSS positions with
respect to the snow surface, and surface roughness. The positioning is based on an industrial
automation laser distance sensor, the SICK DMT10-1111S01. The SICK unit does all time-offlight
processing internally, and passes a raw distance through the serial port at 34,800 baud, as
fast as it can. To log the data in a meaningful way, an Arduino microcontroller is connected with
a combined GPS and data logging shield. This contains a real-time-clock (RTC) with which to
tag the laser distance data; the RTC gets set by the on-board nav-grade GNSS receiver, and thus
the laser data can later be syncronized with the OGRE’s geodetic GPS data. The laser data are
logged to a MicroSD card in a simple text format. Two 12 volt batteries provide 12 V for both the
Arduino and OGRE and 24 V for the SICK laser. An in-box charger allows for charging before
the traverse. The whole Nanuk case is designed to be mounted to the side of the PolyPod sled.

See SLIVERDocumentation for more details. 
