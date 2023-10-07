# xiinux

experimental high performance web application server for linux in c++

intended use:
* compact web application for IoT
* runs on one thread

supports:
* serve files
* serve static content
* serve dynamic content
* post content using ajax
* upload files
* resumable downloads
* ui framework (in progress)
  - focused on back-end development
  - hierarchial structure
  - simple event model for decoupling parent from child elements
  - ajax updates

howto:
* to build run 'make.sh'
* for normal mode run 'xiinux'
* for benchmarking mode run 'xiinux -b'
* for displaying metrics run 'xiinux -m'
* for both benchmarking mode and metrics run 'xiinux -bm'

note. abandoned in favor of [bob](https://github.com/calint/bob)

```

            lines   words   chars
   source:   1510    6037   58605
  gzipped:     40     240   12504

-rwxrwxr-x 1 c 108K okt  4 19:32 xiinux

```