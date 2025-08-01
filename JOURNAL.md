---
title: "USB Time Capsule"
author: "Jaxon"
description: "A USB drive that functions as a time capsule, storing files that can only be accessed after a certain date."
created_at: "2025-07-24"
total_time: "28.5 hours"
---
# 24 July 2025 - Research and Beginning Schematic (3.5 hours)
![Sch](https://hc-cdn.hel1.your-objectstorage.com/s/v3/dc118e1da624370f2bc0077d0b43790ad77fc279_cleanshot_2025-07-26_at_12.50.05_2x.png)
- As I've never A) designed a PCB before and B) have minimal experience with hardware, I decided to start with gathering resources like the official RP2040 guide and watched some video explanations of how things like Decoupling Capacitors work. It took a while to wrap my head around how it worked.
- I started slowly creating the schematic in KiCad, figuring out how everything should be connected.
Although it's only been the first day, it's crazy how much I felt like I learned already. I feel like I'm finally understanding how everything fits together.

# 25 July 2025 - Schematics work (3 hours)
- I continued working on the schematic, and I learned that you can label wires with the same name to connect them. I can't believe I didn't know that before, it's so much better.
- Either way, I made major progress on the schematic and I think I have most of it done.
# 26 July 2025 - Schematic (not) finished (2 hours)
- I finished the schematic, at least I thought I did.
- Turns out I misunderstood how the RTC clock worked, so I have to redo some of the schematic. I started researching how to get the LED to blink for a long time
# 28 July 2025 - Schematic redone (2 hours)
- I completely finished the schematic!
![Sch](https://hc-cdn.hel1.your-objectstorage.com/s/v3/c6572512aa90ad1c502c3402957ed2ccf334bc14_48635.png)
# 29 July 2025 - PCB design (2 hours)
- I placed all the components on the PCB and started routing the wires.
- I had to make a few changes to the schematic, but nothing major.
(I forgot to take a screenshot at this point, but look at the next one to get an idea of how it looked)
# 30 July 2025 - PCB design FINISHED (7 hours)
- I finished routing the PCB, and WOW did it take an age. I guess it's because I didn't really know what I was doing, but still :skull:
- It's really space inefficient, but I don't really care.
- YAY! I'm so close! It's my first PCB design, I just hope I didn't make any mistakes.
![PCB](https://hc-cdn.hel1.your-objectstorage.com/s/v3/da9561fb7693e28b661d9a2c5b9e7349f19660e8_cleanshot_2025-07-30_at_20.36.00_2x.png)
- I also started working on the BOM
# 31 July 2025 - Last Sprint! (9 Hours)
- I finished picking the components, and I made the BOM!
- I added some random art to the PCB from the internet.
![ART](https://hc-cdn.hel1.your-objectstorage.com/s/v3/605c457ba0c42fb18f96c4d72bac313393d39bc5_cleanshot_2025-07-31_at_14.38.44_2x.png)
- I wrote the firmware for the device
- I made some last minute fixes to the PCB