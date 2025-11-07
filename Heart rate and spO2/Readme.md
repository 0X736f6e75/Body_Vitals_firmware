Please find the library used by the following url:

https://github.com/sparkfun/SparkFun_MAX3010x_Sensor_Library

- The following library also contains the algorithm for spo2 calculation as a header file.

- Heart rate monitoring firmware uses the following flow chart for filtering and averaging the instantaneous BPM.


### Legend

  * `[ ... ]` = Start / End / Halt
  * `( ... )` = Process / Action
  * `< ... >` = Decision

-----

### Setup() Flow

This part runs once at the beginning to initialize the hardware.

```
          [ START setup ]
                 |
                 v
        ( Begin Serial @ 115200 )
                 |
                 v
        ( Print "Initializing..." )
                 |
                 v
        ( Initialize particleSensor )
                 |
                 v
        < Sensor found? >
                 | (Yes)
                 v
        ( Configure sensor defaults )
                 |
                 v
        ( Set Red LED amplitude low )
                 |
                 v
           [ END setup ]
                 |
                 +-------------------(No)-----------------+
                                                          |
                                                          v
                                             ( Print "MAX30102 not found" )
                                                          |
                                                          v
                                                   [ HALT (infinite loop) ]
```

-----

### loop() Flow (Main Algorithm)

This is the core algorithm that runs continuously after `setup` is complete.


        [ START loop ]
              |
              v
        ( Read irValue from sensor )
              |
              v
        < Beat detected? > --(No)------------------------------------+
              | (Yes)                                                |
              v                                                      |
        ( Calculate time delta since last beat )                     |
              |                                                      |
              v                                                      |
        ( Update lastBeat time to current time )                     |
              |                                                      |
              v                                                      |
        ( Calculate beatsPerMinute (BPM) )                           |
              |                                                      |
              v                                                      |
        < BPM valid (20-255)? > --(No)-------------------------+     |
              | (Yes)                                          |     |
              v                                                |     |
        ( Store new BPM in rates[] array )                     |     |
              |                                                |     |
              v                                                |     |
        ( Increment & wrap rateSpot index )                    |     |
              |                                                |     |
              v                                                |     |
        ( Loop through rates[] array )                         |     |
              |                                                |     |
              v                                                |     |
        ( Calculate average BPM (beatAvg) )                    |     |
              |                                                |     |
              +------------------------------------------------+     |
              |                                                      |
              +------------------------------------------------------+
              |
              v
        ( Print: IR, BPM, Avg BPM )
              |
              v
        < irValue < 50000? > --(No)-----+
              | (Yes)                  |
              v                        |
        ( Print " No finger?" )         |
              |                        |
              +------------------------+
              |
              v
        ( Print new line )
              |
              v
     ( (Back to START loop) )
