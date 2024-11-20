# R.C. Testout Spec

1. Connect to equipment: `Aurduino`, `Rigol`, `Keithley` upon program startup

## Main Testing Loop
2. Wait until next Trip Unit is connected
3. Program the FTDI chip to say: `ACPro2-RC`
3. Program TU serial number (no L.D. on R.C. trip unit)
4. Program Personality
    a. need details on this; what personality bit should be allowed to optionally be set?
5. short tests
6. Trip Tests:
    1. LT Trip Tests
    2. Inst. Trip Tests
    3. QT Inst Trip Tests
    4. GF Trip Time Test
    5. NOL Trip Time Test
7. Update Database

# TBD
1. Update database; need specific fields in database that should be updated
2. Need to develop exact procedures the fields should be based on; what is the "HMI test"?
3. Hardware Platform Needs defined:
    1. RIGOL 
    2. Keithley
    3. Arduino ???
    4. ???

# STATUS 

1. Make spread sheet showing 32 calibrations for Tim 