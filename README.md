# Tinyserve
Tinyserve is going to be a http(s) server implemented in C++ to host static files. Eventually, it may move to a more dynamic system, but currently will only support a single host.

## Options

The following options will supported at the start of this project, if more options are necessary for my purposes or others, they can be added later.

The configuration file will be a json object with the following structure:

```
{
 "port" : <number>,
 "maximum_queued" : <number>,
 "maximum_threads" : <number>,
 "host" : {
  "static_folder" : <string>
  "default_root" : <string>
 },
 "cache" : {
  "size" : <number>,
  "time" : <number>
 },
 "ssl" : {
  "file" : "string"
 }
}
```

each of the options will be explained below:

1) port: the port that you want the server to listen on
2) maxmimum_queued: the total number of queued requests you want before a send back occurs
3) maximum_threads: maximum threads you want in the pool to service requests
4) host: relates to all things defined for hosting a static site
5) static_folder: the root folder of your static assets
6) default_root: what file you want to show when your site is hit
7) cache: relates to all things in caching your static site in memory
8) size: the maximum number of items you want held in the cache
9) time: the time to live that the items will stay in the cache before they are wiped from memory
10) ssl: relates to all things to setup ssl for your static site
11) file: the location of your certificate
