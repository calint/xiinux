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
   source:   1816    5959   50361
  gzipped:     42     249   11820

-rwxrwxr-x 1 c 64K sep 28 18:38 xiinux

```