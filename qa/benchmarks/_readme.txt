note
* run on ubuntu 23.04 default settings
* nginx 1.22.0 (Ubuntu) used for reference comparison
* nginx is configured with 1 worker thread to mimic single threaded xiinux
* uri '/': xiinux keeps homepage in memory while nginx is the default
  configuration but limited to 1 worker thread

how to benchmark:
  # does all benchmarks, plots and puts the result in directory "report"
  ./do-report.sh

or
  # does all benchmars, generates result files
  ./do-all-benchmarks.sh

or
  # benchmarks a particular server
  ./do-benchmark.sh nginx localhost 80
  ./do-benchmark.sh xiinux localhost 8088

or try
  # benchmarks url
  hey -n 1 -c 1 http://localhost:8088/
  hey -n 1 -c 1 http://localhost:8088/qa/files/small.txt
  hey -n 1 -c 1 http://localhost:8088/qa/files/far_side_dog_ok.jpg
