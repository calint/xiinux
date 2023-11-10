# xiinux

experimental high performance web application server for linux in c++

intention:
* compact web application server
* backend for browser based application
* deployed as server 
* runs on one thread

supports:
* serve files
* serve static content
* serve dynamic content
* post content using ajax
* upload files
* resumable downloads
* ui framework
  - focused on back-end development
  - hierarchial structure
  - simple event model
  - ajax updates

howto:
* to build run 'make.sh'
* for normal mode run 'xiinux'
* for benchmarking mode run 'xiinux -b'
* for displaying metrics run 'xiinux -m'
* for both benchmarking mode and metrics run 'xiinux -bm'

```

            lines   words   chars
   source:   2057    7935   78149
  gzipped:     69     347   16913

-rwxrwxr-x 1 c 325K okt  7 12:28 xiinux

```