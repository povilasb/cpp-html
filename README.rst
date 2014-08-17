=====
About
=====

Master:
.. image:: https://travis-ci.org/povilasb/cpp-html.svg?branch=master
Develop:
.. image:: https://travis-ci.org/povilasb/cpp-html.svg?branch=develop


*cpp-html* is a c++ library for modern c++ (c++11 and up) compilers. It
is used to parse and manipulate HTML documents with a DOM like API.

Currently Unicode support is untested. Should work well with ASCII symbols.
Also there's no functionality to save HTML document from DOM tree.
See **TODO** for what's comming next.


Goals
=====

* Support for different character encodings.
* Ability to save DOM tree to HTML document.
* Almost full DOM support.


Tested on
=========

* GCC 4.7.2 on Debian wheezy 7.

Please submit success or failure results, if you've tried to compile with
different compilers.


Dependencies
============

None.


Test
----

* googletest. Download latest version from
  http://code.google.com/p/googletest/downloads/list. These tests were written
  with 1.7.0. Unzip to './lib'.


Documentation
-------------

* doxygen: ``sudo apt-get install doxygen``.
