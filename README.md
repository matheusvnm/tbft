# Thread and Boosting Frequency Throttling (TBFT)

Thread and Boosting Frequency Throttling (TBFT) is an easy to use OpenMP framework that is completely transparent to both the developer and the end-user. Without any recompilation, code modification or OS installitions, it is capable of finding, automatically and at run-time, the optimal number of threads for the parallel regions and at the same time controls the turbo boosting mode to optimize the performance and energy consumption. 

### List of files contained in TBFT
---

* boost.sh              -  tbft algorithm to control the turbo boosting techniques.
* tbft.c                -  tbft functions implementation
* tbft.h                -  tbft header
* env.c                 -  OpenMP internal controler variables
* libgomp.h             -  libgomp header
* libgomp_g.h           -  libgomp header
* parallel.c            -  libgomp header
* Makefile.in           -  OpenMP libgomp makefile.in
* Makefile.am           -  OpenMP libgomp makefile.am


### TBFT's dependencies.

1. MSR Module (only in AMD Version).
2. Intel RAPL (only in Intel Version).
3. ACPI cpu-freq driver.
4. GCC 9.4+.


### How to install TBFT?
---

1. Choose the version you are going to use based on your processor (Intel or AMD).
2. Copy all files into the gcc libgomp directory:
      - cp processor_version/* /path/gcc-version/libgomp.
      - cp shared/* /path/gcc-version/libgomp.
3. Compile the GCC using Make && Make install:
      - cd /path/gcc-version/libgomp
      - make
      - make install


### How to use TBFT?
---

1. Export the library path, the full boost.sh path and set TBFT's environment variable:
      - export LD_LIBRARY_PATH=/path-to-gcc-bin/lib64:$LD_LIBRARY_PATH
      - export OMP_URANO_BOOST_PATH=/gcc-version/libgomp
      - export OMP_URANO=TRUE
      
2. Execute the application.

3. (OPTIONAL) Export the IPC Threshold and Time Interval for the Turbo Mode Engine.
      - export IPC_TARGET=0.6 (Can be any value in the format %d.%d)
      - export TIME_TARGET=1 (Can be any value in the format %d)
x
### Acknowledgement
---

TBFT has been mainly developed by Sandro Matheus Vila Nova Marques (sandro-matheus@hotmail.com) during his BSc/MSc. under supervision of Arthur Francisco Lorenzon (aflorenzon@unipampa.edu.br).

When using TBFT, please use the following reference:

XXXXXXX
