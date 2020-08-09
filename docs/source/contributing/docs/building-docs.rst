Building docs
=======================

This page contains how to build this Cutter documentation.

Requirements
------------

You will need:

* doxygen
* python3
* pip3
  * sphinx
  * breathe
  * recommonmark

On Debian-based Linux distributions, all of above packages can be installed with this command.

.. code:: sh

   sudo apt install make doxygen python3-pip doxygen
   sudo pip3 install sphinx breathe recommonmark

Then you can build documents with the following commands.

.. code:: sh

   cd cutter/docs/
   make html

If you do not need API documentation, type ``make quick`` instead of ``make html``.
   
You can find the generated html files at cutter/docs/build.
