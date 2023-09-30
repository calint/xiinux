# xiinux

experimental high performance web application server for linux in c++

intended use:
* compact web application for IoT
* runs on one thread

supports:
* serve file
* serve static content
* serve dynamic content
* post data using ajax
* upload file
* resumable download

howto:
* to build run 'make.sh'
* for normal mode run 'xiinux'
* for benchmarking mode run 'xiinux -b'
* for displaying metrics run 'xiinux -m'
* for both benchmarking mode and metrics run 'xiinux -bm'

note. abandoned in favor of [bob](https://github.com/calint/bob)

```

            lines   words   chars
   source:   2083    6813   59748
  gzipped:     45     275   13684

-rwxrwxr-x 1 c 76K sep 30 23:37 xiinux

```