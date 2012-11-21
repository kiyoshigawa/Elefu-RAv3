Elefu RA v3

This is the code repo for the Elefu RA Board version 3. It has the correct pins.h setting for the RA v3 board, and some added changes in configuration.h and configuration_adv.h to work with our control panel, lighting and sound controllers.

If you are one of our kickstarter backers or bought a RA board from Elefu, this is likely the correct code repository for you.
To use it, you simply need to download the entire repository as a zip file, extract it, and then open the 'Marlin.pde' file in your Arduino IDE, plug your RA board into your USB port, and upload your changed code like you would any Arduino program.
To change settings for your specific printer, edit the 'configuration.h' and 'configuration_adv.h' files and re-upload the code fromt he arduino IDE.

RA's code is based almost entirely on Marlin (v1 RC2) ( https://github.com/ErikZalm/Marlin/ ) with a few changes made by me to work with the RA board and its peripherals. It still works just like Marlin for almost all printer functions, so the readme for Marlin will work for explaining how the firmware works for the most part.
I have added comment sections for anything I added or changed that explain what I have done. They are prefaced with //ELEFU so they should be easy to find in a search. 

If you find any bugs in the code or have any questions, I can be reached at timanderson at elefu dot com.

Hope you all enjoy RA!

Cheers,
     -Tim Anderson
      Elefu.com
