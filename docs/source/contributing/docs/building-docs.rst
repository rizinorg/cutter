Building docs
=======================

This page explains the steps that are needed to build Cutter's documentation.

Requirements
------------

You will need:

* doxygen
* python3
* pip3

  * sphinx
  * breathe
  * recommonmark

On Debian-based Linux distributions, the packages above can be installed with the following command.

.. code:: sh

   sudo apt install make doxygen python3-pip doxygen
   sudo pip3 install sphinx breathe recommonmark

Then, you can build the documentation with the following commands:

.. code:: sh

   cd cutter/docs/
   make html

.. tip::

   If you do not need API documentation, type ``make quick`` instead of ``make html``.
   
You can find the generated html files at ``cutter/docs/build``. Open ``cutter/docs/build/html/index.html`` with your browser to visit the index file of your local copy of the documentation.
