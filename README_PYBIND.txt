To test the current pybind setup, proceed as follows:

$ mkdir pybind-build
$ cd pybind-build
$ cmake .. -DPYTHON_LIBRARY_DIR="<absolutepathtorepositoryroot>/try-pybindings" -DPYTHON_EXECUTABLE="/usr/bin/python3"
 (or wherever your python3 lives)
$ make
$ make install
$ cd ../try-pybindings
$ ls
pydownward.cpython-310-x86_64-linux-gnu.so  test.py
$ cat test.py

Put some output.sas file into the directory. The test only runs enforced
hill-climbing, which has a high likelihood to NOT solve a task. It can for
example solve the first driverlog task, so if in doubt, use that one.

Enjoy:
$ python -i 
(The plan is stored in variable "plan", have a look!)
