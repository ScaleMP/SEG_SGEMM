# SGEMM BENCHMARK FOR LARGE MEMORY SYSTEMS
## INTRODUCTION
This SGEMM workload demonstrates the multiplication of very large (single precision) matrices, which is the corner stone of many algorithms, leveraging Intel(R) Math Kernel Library (MKL).
In order to improve computation locality, the algorithm is performing the multiplication in segments.
## BENCHMARK DESCRIPTION
The benchmark at hand is using matrices of various growing sizes (up to ~90% of system meory) and measuring the GFlop/s achieved.
MATRICES SIZE CALCULATION
The dimensions of the 3 matrices are as follows:
- A: size x seg
- B: seg x size
- C: size x size

We define:  size = factor x seg (where the factor changes between runs according to the desired memory footprint).
As each cell in the matrices requires 4 bytes, we can calculate the total memory  that is required for the process:

Total memory = (2 x (size x seq) + size x size) x 4 bytes

= size x (2 x seg + size) x 4bytes
= factor x seg x (2 x seg + factor x seg) x 4bytes

The following table presents the the total memory required for various factors according to the calculation described above (for seg = 43211 ) :

|Factor|Memory Footprint [GB]|          |Factor|Memory Footprint [GB]|
|---|---|----------|---|---|
|1|21|          |12|1,169|
|2|56|          |13|1,356|
|3|104|          |14|1,558|
|4|167|          |15|1,774|
|5|243|          |16|2,003|
|6|334|          |17|2,247|
|7|438|          |18|2,504|
|8|556|          |19|2,775|
|9|689|          |20|3,061|
|10|835|          |21|3,360|
|11|995|          |22|3,673|

## SYSTEM AND BENCHMARK INSTALLATION AND CONFIGURATION
The instructions below assume installation on CentOS 7.3 or newer, but can be easily adjusted to run on other distributions.
The following packages are required to run the SGEMM workload, and can be installed if needed using the following commands:
```
  # yum install
  # yum install numactl
  # yum install time
```

### BENCHMARK CONFIGURATION
1.	Create a folder named “sgemm”: 
```
  # mkdir /root/sgemm
```
2.	Step into the “redis” folder: 
```
  # cd /root/sgemm
```
3.	Create a file named mkl-seg.c with the code from the appendix “mkl-seg.c code” (or from the workload directory/tar).

4.	Compile the code using the Intel compiler by pointing to the location of the intel compiler location, or alternatively use the statically compiled executable “mkl-seg” (from the workload directory / tar).
```
  # . /opt/intel/bin/compilervars.sh intel64
  # icc -O3 -ipo -static -g -fopenmp -mkl -o mkl-seg mkl-seg.c
```
5.	Put a file named run_sgemm.sh with the code from the appendix “run_sgemm.sh script” (or from the workload directory/tar).

6.	If you are not using a statically linked executable, uncomment and adjust the line pointing to the intel compiler variables:
```
  # INTEL_COMPILER_VARS=/opt/intel/bin/compilervars.sh
```

## RUNNING THE BENCHMARK
### INITIALIZING THE BENCHMARK
The run script has several options to run the workload with various memory sizes (factors):
- short: 	Use two values for the test: 1 and the maximal value for fac that fits into 90% of system memory
- medium: 	**Default option**: from 1 to the maximal value by powers of 2
- full:   	The entire range from 1 to the possible maximum is tested
- list:   	Explicit (quoted) list is set by the user in the next parameter (e.g. “1 2 8”)

To run the script with the default option:
```
  # cd /root/sgemm
  # ./run_sgemm.sh
```
Or with a specific option, for example:
```
  # cd /root/sgemm
  # ./run_sgemm.sh short
```

### COLLECTING RESULTS
As we are interested in the overall average GFlop/s achieved per run, it can be obtained with:
```
  # grep "Init time" log-gemm*.txt | grep GFlop
```




*THIS SOFTWARE IS PROVIDED BY SCALEMP "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SCALEMP BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.*








