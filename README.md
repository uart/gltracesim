# GLTraceSim

GLTraceSim is a graphics tracing and replay framework that enables to explore system-level effects on heterogeneous CPU+GPU memory systems. By efficiently generating GPU memory access traces for modern graphics applications, GLTraceSim replays them through a high-level model or a detailed simulator to explore effects in bandwidth, cache misses, scheduling and performance. GLTraceSim is built upon publicly-available tools

See [uart/gltracesim] for more information.

![gltracesim-overview]

## Prerequisites

### C/C++

* [make][]
* [cmake][] — CMake build system
* [automake][]
* [libtool][]
* [SCons][]

### Python

* [python-setuptools][] — Python setup tools library

## Setting up external packages

### Preparing the build folder

Start by cloning the following repository

    git clone https://github.com/uart/gltracesim.git
    
After that, you will see the following folder structure

    gltracesim/
                config
                gltracesim
                traces
                tools
* `config` -- Folder containing scripts to launch batch jobs over a set of traces.
* `gltracesim` -- sources for GLTraceSim's trace generator and high level model
* `traces` -- folder for the OpenGL calls trace files. You can get them [here](https://www.dropbox.com/sh/a9khvc51krx8h6t/AAB3UUDLbHh_FONua-z-2xuta?dl=0), make sure to uncompress them under the `traces/` folder.
* `tools` -- sample scripts that process the output data. These scripts show how to extract the output data from the protobuffer files 

GLTraceSim requires several external packages and libraries built and installed (see [External packages and libraries](#external-packages-and-libraries). By default, the compilation scripts will assume all of them will be unpacked in `gltracessim/ext` and installed in `gltracesim/ext/local/`

Feel free to change the installation directories, but make sure to set `APITRACE_HOME`, `JSONCPP_HOME`, `PROTO_HOME`, `MESA_HOME`, and `PYTHONPATH` accordingly for SCons to work. All these paths are defined in `gltracesime/conf.rc`. **Make sure to set up the** `BASE` and `BASE_LOCAL` **variables correctly in** `gltracesim/conf.rc` **file, and source it before using the tool.** You can do this by running:

    source /path/to/gltracesim/gltracesim/conf.rc

### External packages and libraries

* [apitrace]
        ```git clone https://github.com/apitrace/apitrace in ext/```
* [libpng++], [libpng-12]
* [libjpeg-dev]
* [png++]
* [mesa] -- Mesa 3D Graphics Library. **Please use version mesa-11.3.0-devel**
* [protobuf] -- Google Protocol Buffers. **Please use version 3.0 or 3.2**
* [jsoncpp]-- clone from official github , unpack and build in `ext/jsoncpp`. To compile `jsoncpp` you need [meson] (>0.41) and [ninja]. Get [meson] from the official website, or ```sudo pip3 install meson```. Get [ninja] from the official website, or ```sudo apt-get install ninja-build```.

## GLTraceSim Overview

Our tool has 3 components: a Pin-based Trace Generator (*gltracesim-generate*), a High-Level Model (*gltracesim-analyze*) and a Gem5 extension (not published yet).

The GLTraceSim Generator feeds an OpenGL trace, and replays it using a software renderer. During this process, the memory accesses of the render threads are captured, along with plenty of other information, which is saved in a **GLTraceSim trace** (see [GLTraceSim Traces](#gltracesim-traces)).

The GLTraceSim High Level Model (HLM onwards) replays the GLTraceSim traces to analyze cache/memory behavior. The Analyzer implements different types of caches with different configurations. While the GLTraceSim trace is replayed, the cache state is dumped over time to a specified folder to protobuffer files, which can be used for example to compute the cache miss ratios as a function of cache size.

GLTraceSim's Gem5 Extension is an addition to the Gem5 Simulator that replays the GLTraceSim traces, feeding the memory access into Gem5's memory system, to obtain performance and bandwidth numbers of these applications running under a particular hardware architecture.

We provide OpenGL call traces for all benchmarks in the [GFXBench] and [Phoronix] suite, as well as several websites rendered using the Chrome Browser. You can get them [here](https://www.dropbox.com/sh/a9khvc51krx8h6t/AAB3UUDLbHh_FONua-z-2xuta?dl=0). Make sure to place those bechmarks under `gltracesim/traces`.

## Building GLTraceSim

To build the trace generator (pintool):

    scons mode=debug++ -j4 proto
    scons mode=debug++ -j4 libgltracesim.so

To build the High Level Model (analyzer)

    scons mode=debug++ -j4 gltracesim-analyze.o 

## Using the tools directly

### 1. Generate GLTraceSim traces  

The following example generates a GLTraceSim trace from replaying an OpenGL trace for the benchmark *cnn*. The GLTraceSim trace is saved in `output/cnn`, and it is used in the succeeding examples for replaying using the High Level Model.

    ./gltracesim-generate -i ../traces/cnn.gltrace \
        -o ../output/gltracesim-traces/cnn \
    
* `./gltracesim-generate` - GLTraceSim Trace Generator
* `-i ../traces/cnn.gltrace` - input OpenGL trace for apitrace
* `-o ../output/cnn` - output folder where the GLTraceSim trace is going to be saved 

![gltracesim-generate-demo]

### 2. Replaying using the High Level Model

The following command replayes the GLTraceSim trace of the *cnn* benchmark through the GLTraceSim High Level Model

    ./gltracesim-analyze --schedular fcfs \
            -i /path/to/input/traces/cnn \
            -f 110 -w 112 \
            -n 1 \
            -m /path/to/gltracesim/config/base.json \
            -o ../output/hlm-results/cnn \
            -d Init,Warn,GpuEvent

* `./gltracesim-analyze` - GLTraceSim Cache High Level Model
* `-- schedular fcfs` - Tile scheduling policy (for tasks inside one scene). Choose between `FCFS`, `RANDOM` or `Z`.
* `-i /path/to/input/traces/cnn` - input GLTraceSim trace
* `-f 110` - fast forwards 110 frames (initialization)
* `-w 112` - warmup until frame 112
* `-n 1` - number of GPU multicores
* `-m /path/to/gltracesim/config/base.json` - Cache configuration file (cache parameters)
* `-o ../output/hlm-results/cnn` - output folder where the GLTraceSim HLM results are going to be saved
* `-d Init,Warn,GpuFrameEvent` - debug/print events (verbosity). You can check the source files for all the different event types. In this case, this will print messages for the initialization, the warnings, and for each GpuFrame event.

![gltracesim-analyze-demo]


### 3. Extracting data

The analyzer will dump all the data from the execution/replay of the trace to the output folder.

For our previous example where the output folder was `output/cnn`, you will see the following files in the directory

    0.core_stats.pb.gz
    0.job_stats.pb.gz
    0.rsc_stats.pb.gz
    0.stats.pb.gz
    stats.pb.gz
    orig.resources.pb.gz
    orig.frames.pb.gz
    orig.scenes.pb.gz
    orig.jobs.pb.gz
    orig.opengl.pb.gz
    orig.stats.pb.gz
    config.json
    output_schedule.pb.gz
    
The prefix 0 indicates the core ID. Since we had a single core execution there is only 0 as prefix. If you replay with 4 cores, you will see multiple files `0.core_stats.pb.gz`, `1.core_stats.pb.gz`, etc.

The files `orig.*.pb.gz` are a exact copy of the frames, scenes, jobs, resources and OpenGL calls from the traces used to replay. 

The `core_stats` file contains dumps of the cache state over time each time the core finishes a job. The `job_stats` file contains the **combined** Cache state after each job was replayed, regardless of the core executed. The `stats.pb.gz` contains the Cache state dumped over time after **any** job finishes in any of the cores. Note that all these files contain the same information (saving the cache state), but dumped at different moments for your convenience.

The `config.json` file contains the configuration parameters used for replaying the trace. The `output_schedule.pb.gz` contains the resulting schedule of the scenes and job IDs after the replay.

You can find the format for these files under `src/proto`. We provide sample scripts under the folder `tools` to dump/extract the data from these files.

The example `dump-jobs.py` and `dump-dpf.py` print the cache statistics and bandwidth consumption for each of the jobs that was replayed.

    cd tools/
    python dump-dpf.py -i /path/to/output/files/benchmark/ -o ./output

## GLTraceSim traces

### Overview of a trace file

For our *cnn* example above, the GLTraceSim trace created by `gltracesim-generate.py` will look as follows

    cnn/
        frames.pb.gz
        scenes.pb.gz
        jobs.pb.gz
        resources.pb.gz
        stats.pb.gz

### Frames

Frames, when rendered, are divided into smaller processes called *scenes* (read [3] for all the details). The `frames.pb.gz` file is a protobuffer file, that contains, the information (serialized) about all the frames as they were executed into `FrameInfo` messages.

The format for the `FrameInfo` messages is

    message FrameInfo {
        uint32 id = 1;              // Frame ID
        repeated uint32 scene = 2;  // Scene ids
        }

You can check the full specification in the file `proto/frame.proto`.
If you query the `frames.pb.gz` file, you will get messages similar to this

    { id = 410, { 1, 2, 3, 4} } ,
    { id = 411, { 1, 2, 3, 4, 5, 6 } } , 
    ...

This means that Frame number 410 was rendered by scenes 1 to 4 (each of them with unique ids as well), and Frame 411 needed 6 scenes to be rendered. Note that Scene 1 of Frame 410 is not the same as Scene 1 of Frame 411. Even though they *might* be the same shader program, they are different instances of it.

All the information about the scenes can be obtained from the `scenes.pb.gz` file.

You can use the script `frame-print.py` in the `tools` folder to display the content of this messages.

    cd tools
    python frame-print.py -i /path/to/gltraces/benchmark/frames.pb.gz
    
You can also use the `frame-print.py` script as a sample to extract the data from the GLTraceSim traces and use it for further analysis or processing.

### Scenes

Similarly to the `frames.pb.gz` file, the information about the scenes executed is serialized with `SceneInfo` messages into `scenes.pb.gz`.

    message SceneInfo {
        uint32 id = 1;                      // Scene ID
        uint32 frame_id = 2;                // Frame ID
        uint32 width = 3;                   // Width [tiles]
        uint32 height = 4;                  // Height [tiles]
        repeated uint32 job = 5;            // Job ids
        repeated uint64 opengl_call = 6;    // OpenGL calls
        }

Each scene has an ID, a corresponding Frame ID, and the number of tiles in each dimension (x-y) given by width and height. A series of job IDs indicate the IDs of the tasks in that particular scene. Finally, a list of all the OpenGL calls performed by that scene is also included.

You can use the script `scene-print.py` in the `tools` folder to display the content of this messages.

    cd tools
    python scene-print.py -i /path/to/gltraces/benchmark/scenes.pb.gz
    
You can also use the scene-print script as a sample to extract the data from the GLTraceSim traces and use it for further analysis or processing.

### Jobs

In `src/proto/job.proto` you will find the following format for the GLTraceSim job traces

    enum JobType {
        MISC_JOB = 0;
        DRAW_JOB = 1;
        TILE_JOB = 2;
    }

    message JobInfo {
        uint32 id = 1;          // Resource ID
        uint32 dev_id = 2;      // Device ID [ CPU, GPU ]
        JobType type = 3;       // Type
        uint32 frame_id = 4;    // Frame ID
        uint32 scene_id = 5;    // Scene ID
        uint32 x = 6;           // X-coord
        uint32 y = 7;           // Y-coord
    }

Each *job* is a taks that is executed non pre-emptively during the replay using the HLM or the simulator. Each job has a unique job ID, and *device* ID which indicates if it is a CPU or GPU job.

The *type* field distinguishes between *DRAW*, *TILE*, and *MISC* jobs. *DRAW* jobs are a series of jobs executed sequentially at the beginning of each scene, for initialization. After all *DRAW* jobs, several independent *TILE* jobs execute in parallel on the multiple GPU multicores. Finally *MISC* jobs are smaller jobs issued by OpenGL for cleanup or sincronization. They execute a very small number of instructions and use almost no data, so in most cases can be disregarded.

Each JobInfo message also indicates the corresponding Scene and Frame of the job, as well as the corresponding X-Y coordinate in the screen.

You can use the script *job-print.py* in the *tools* folder to display the content of this messages.

    cd tools
    python job-print.py -i /path/to/gltraces/benchmark/jobs.pb.gz
    
You can also use the jobs-print script as a sample to extract the data from the GLTraceSim traces and use it for further analysis or processing.

### Resources

The information about all memory resources used is located in the `resources.pb.gz` file of the GLTraceSim trace. You can find a description of the format under `src/proto/resource.proto`. Resources can be of different types described by the ResourceFormat message in resources.proto. 

You can use the script `resource-print.py` in the `tools` folder to display the content of this messages.

    cd tools
    python resource-print.py -i /path/to/gltraces/benchmark/resources.pb.gz
    
You can also use the resource-print script as a sample to extract the data from the GLTraceSim traces and use it for further analysis or processing.

## Using the tools with launch scripts in *config/*

Under the `config/` folder, we provided scripts to run both the trace generator and analyzer at once based on a list of traces (`traces.py`)

### 1. Batch Trace Generation

You can generate GLTraceSim traces for all benchmarks in `traces.py` with the following command

    python run-traces.py \
        --cluster local \
        --no-fast-forward \
        -o output_folder \
        --input-dir ../traces/ 
    
* `./run-traces.py` - GLTraceSim trace generator launcher command
* `--cluster local` - changes the configuration/environment variables depending on the machine. `local` is the default machine using the default installation directories under `/ext/local`. You can adapt these scripts adding new configurations, for example, if running under a cluster, adding `--cluster my_cluster_name`.
* `--no-fast-forward` -- runs the traces without fastforwarding any frame.
* `-o, --output-dir` -- output directory for GLTraceSim own memory traces.
* `-i, --input-dir` -- input directory for the memory traces.

The `traces.py` file not only contains the name of the traces, but also contains information about the **initizalization** frames, used for fast-forwarding, and stop frames.

You can modify the `traces.py` file to include your own applications, or create a custom `my_traces.py` module, specifying the names of the applications as well as the starting/init/ending frames. If you choose the latter one, make sure to include the following line on the `run-generator.py` and `run-analyzer.py` scripts

    import my_traces as traces

### High Level Model

    python run-analyzer.py \
        --cluster local \
        --no-fast-forward \
        -o output_folder \
        --input-dir traces \
        -m base.json \
        -n 4 \
        --filter telemetry_reddit \
        --schedular fcfs \
        --pretend --sbatch

* `./run-analyzer.py` - GLTraceSim Analyzer launcher command
* `--cluster local` - changes the configuration/environment variables depending on the machine.
* `--no-fast-forward` -- runs the traces without fastforwarding any frame.
* `-o, --output-dir` -- output directory for GLTraceSim own memory traces.
* `-i, --input-dir` -- input directory for the memory traces.
* `-m base.json` -- JSON file containing the cache configuration, including type of cache and parameters (size, associativity, etc)
* `-n 4` -- number of cores
* `--filter telemetry_reddit` -- only runs the subset of benchmarks specified by filer. If no filter is specified, all the benchmarks specified in traces.py will be executed.
* `--schedular fcfs` -- defines the scheduling policy for tiles (tasks) within a scene of a frame. Policies implemented with the release include `FCFS`, `RANDOM` and `Z-scheduling`. For more information about the definition of tiles, scenes and frames, scheduling, as well as their memory implications, please refer to [3].
* `--pretend` -- Does not launch the jobs but prints a list of the jobs that are going to be launched.
* `--sbatch` -- uses sbatch to launch parallel jobs (for [SLURM])

## Publications using GLTraceSim 

If you use GLTraceSim for your research, please cite [1] and [3].

#### 2018

[3](https://www.researchgate.net/publication/323643947_Behind_the_Scenes_Memory_Analysis_of_Graphical_Workloads_on_Tile-based_GPUs) -- **Behind The Scenes: Analyzing Graphical Workloads on Task-Based GPUs**. Germán Ceballos, Andreas Sembrant, Trevor E. Carlson, and David Black-Schaffer. *In IEEE International Symposium on Performance Analysis of Systems and Software (ISPASS'18)*.

#### 2017

[2](http://ieeexplore.ieee.org/document/8167761/) - **Analyzing Graphical Workloads on Task-Based GPUs**. Germán Ceballos, Andreas Sembrant, Trevor E. Carlson, and David Black-Schaffer. *In IEEE International Symposium on Workload Characterization (IISWC'17)*.

[1](http://ieeexplore.ieee.org/document/8167756/) - **A Graphics Tracing Framework for Exploring CPU+GPU Memory Systems**. Andreas Sembrant, Trevor E. Carlson, Erik Hagersten and David Black-Schaffer. *In IEEE International Symposium on Workload Characterization (IISWC'17)*.

[make]: https://www.gnu.org/software/make/
[cmake]: http://www.cmake.org/
[automake]: https://www.gnu.org/software/automake/
[libtool]: https://www.gnu.org/software/libtool/
[boost]: http://www.boost.org/
[SCons]: http://www.scons.org
[protobuf]: https://code.google.com/p/protobuf/
[jsoncpp]: https://github.com/open-source-parsers/jsoncpp
[apitrace]: https://github.com/apitrace/apitrace
[png++]: http://www.nongnu.org/pngpp/
[mesa]: https://www.mesa3d.org
[SLURM]: https://slurm.schedmd.com/sbatch.html
[python-setuptools]: https://pypi.python.org/pypi/setuptools

[libjpeg-dev]: http://libjpeg.sourceforge.net
[libpng++]: libpng++
[libpng-12]: https://packages.debian.org/jessie/libpng12-0
[ninja]: https://pypi.python.org/pypi/ninja/1.7.2
[meson]: http://mesonbuild.com

[GFXBench]: https://gfxbench.com/result.jsp
[Phoronix]: https://www.phoronix-test-suite.com

[uart]: http://www.it.uu.se/research/group/uart/
[uart/gltracesim]: http://www.it.uu.se/research/group/uart/gltracesim


[gltracesim-overview]: http://www.it.uu.se/research/group/uart/gltracesim_overview2.png
[gltracesim-generate-demo]: http://www.it.uu.se/research/group/uart/gltracesim_generate_demo2.png
[gltracesim-analyze-demo]: http://www.it.uu.se/research/group/uart/gltracesim_analyze_demo.png
[gltracesim-traces]: https://www.dropbox.com/sh/a9khvc51krx8h6t/AAB3UUDLbHh_FONua-z-2xuta?dl=0
