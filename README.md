#SGEMM BENCHMARK FOR LARGE MEMORY SYSTEMS
##INTRODUCTION
This SGEMM workload demonstrates the multiplication of very large (single precision) matrices , which is the corner stone of many algorithms, leveraging Intel(R) Math Kernel Library (a.k.a. MKL).
In order to improve computation locality, the algorithm is performing the multiplication in segments.
##BENCHMARK DESCRIPTION
The benchmark at hand is using matrices of various growing sizes (up to ~90% of system meory) and measuring the GFlop/s achieved.
MATRICES SIZE CALCULATION
The dimensions of the 3 matrices are as follows:
A: size x seg
B: seg x size
C: size x size
We define:  size = factor x seg (where the factor changes between runs according to the desired memory footprint).
As each cell in the matrices requires 4 bytes, we can calculate the total memory  that is required for the process:
Total memory = (2 x (size x seq) + size x size) x 4 bytes
= size x (2 x seg + size) x 4bytes
= factor x seg x (2 x seg + factor x seg) x 4bytes
The following table presents the the total memory required for various factors according to the calculation described above: 
seg = 43211 
##SYSTEM AND BENCHMARK INSTALLATION AND CONFIGURATION
The instructions below assume installation on CentOS 7.3 or newer, but can be easily adjusted to run on other distributions.
The following packages are required to run the SGEMM workload, and can be installed if needed using the following commands:

  $yum install
  $yum install numactl
  $yum install time
