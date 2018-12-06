# NetRun #

NetRun is a web front end to real compiled code running on the server,
including C++, assembly, and CUDA.

https://www.cs.uaf.edu/~olawlor/papers/2011/netrun/lawlor_netrun_2011.pdf

The major pieces:
* www: Public frontend webserver files, including help and UI files.
* netrun/bin/run.cgi: The main frontend perl script.
* netrun/serve/sandrun_export: The HMAC-authenticated pipe used to connect frontend and backend.  (Should probably be SSH nowadays.)
* netrun/support: Per-run project makefile.
* netrun/serve/s4g_chroot: Actual sandbox used to run binary programs on backend server.

Lots of supporting documentation still needs to be built!

It was built by Dr. Orion Lawlor (lawlor@alaska.edu) starting in 2005, 
mostly for his computer architecture and programming courses.

License: public domain
