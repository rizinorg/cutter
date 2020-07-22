Contributing code to Cutter
============================

Thank you so much for your interest in contributing to Cutter! Contributors like you are a treasured resource for Cutter and their contributions have made the project what it is. And so, we appreciate everyone who gives the community the gift of their time <3.

The following section in our documentation is *not* intended to be comprehensive but rather a quick guide to walk you through the basic flow of contributing to Cutter. For more thorough documentation, follow the links on this page and read our Development Guidelines.

Clone and compile Cutter
-------------------------
The first step before starting to add code to Cutter is to build it on your environment. Whether it is Linux, Windows, or macOS, we have you covered and detailed the instructions to compile Cutter on our :doc:`/building`. Once you are done and Cutter is built successfully, test that it works correctly and continue to the next steps.

.. tip::
  If you are facing issues with building Cutter on your environment, make sure you didn't miss anything in the documentation. Specifically, check the :ref:`Building:Troubleshooting` section.

.. tip::
  If you need help configuring your development environment, make sure to read our :doc:`instructions, recommendations and tips <ide-setup>` for setting up a Cutter development environment for popular IDEs.



Find something to work on
--------------------------
Some will reach this page with a clear idea of what issue they want to fix or what feature they wish to implement. But some would simply want to help Cutter getting better while doing open-source, without having a specific thing in mind. If you already have something in mind - great! Move forward to the next section. If you don't have anything specific, stick with us a little bit more.

The issues and the feature-requests of Cutter are listed and tracked on our `Github Issues <https://github.com/radareorg/cutter/issues>`_ page. Don't get scared by the number of issues open, we will learn how to filter them to find those which fit you best.

.. tip::
  **Fix your pet peeve!** Anything specific that annoys you about Cutter? Fantastic! This can be a great place to start.


Organizing issues
*******************
In order to organize the hundreds of Issues opened on Cutter, we use several features of Github for project management. Get yourself familiar with the following features, it will help you filter the issues.

Labels
^^^^^^^^
Tagging issues and pull requests with labels allows us to quickly search for them later. We use labels to describe the type of the issue, the feature it belongs to, the difficulty, and even its priority. We recommend you to start from issues labeled as `good-first-issue <https://github.com/radareorg/cutter/issues?q=is%3Aopen+is%3Aissue+label%3A%22good+first+issue%22>`_. These issues were tagged by us as suitable for new contributors.

Projects
^^^^^^^^^^
We use `project boards <https://github.com/radareorg/cutter/projects>`_ to gather together tasks and issues for specific features or problems. For example, we have dedicated project boards for Debugger, Hexdump widget, Decompiler widget, and High DPI support. If you are interested to take part in developing a major feature, or you want to get a broader look at a progress of a specific part in Cutter, this can be a good place to visit.

Milestones
^^^^^^^^^^^^

When starting working on a new release, we gather together the bug and feature-requests that we consider of high-priority into a `Milestone <https://github.com/radareorg/cutter/milestones>`_. If you want to work on a feature or fix a bug that is needed and prioritized for the next release, then check out the open Milestone,.


Assigning Issues
^^^^^^^^^^^^^^^^^^

Before choosing an issue to work on, make sure it is not assigned to anyone. If it is, you can comment and ask if this person is intended to work on the issue. Some assigned issues are abandoned by their assignee and can be picked up by other contributors.



Get familiar with the current situation
----------------------------------------

Run Cutter and get familiar with how the functionality you want to modify works currently. For example, if it is a dialog, make sure you understand its role and how our users are using it. If it is a bug, try to repeat it using the instructions from the bug report.

.. tip::
  If you are inexperienced with binary analysis tasks, you can always ask other developers in the team about the feature and how it should be used. The team and the community will be happy to help and instruct you with everything you need to know. 

Find the source files implementing the feature you want to work on. By doing this, practice searching code in your editor, this is an important skill. We suggest searching for files with the name of the feature. For example, if you are interested in improving the "Sections" widget, you can find the source code of this feature in `SectionsWidget.cpp` and `SectionsWidget.h`. Alternatively, you can try and look for specific unique strings that exist in the dialog, widget, or feature you want to improve. Usually, when searching the entire code base for these strings, you'll land on a file related to this feature, whether it is a ``.cpp`` file or a ``.ui`` file. From there it will be easy to navigate your way to the right place.


Work on your feature or bug-fix 
-------------------------------

If you are experienced with such tasks, go ahead - we leave this in your hands. Otherwise, we recommend you come up with a plan for things that need to be done to solve this bug or implement this feature. Discuss your plan in the relevant issue on GitHub.

.. important::
   Before starting coding, make sure to get yourself familiar with our comprehensive documentation for our `coding style, conventions, and guidelines <development-guidelines.html>`_.


If you don't know how to implement something, check if any of the existing code implements similar behavior in the same widget or similar widgets. If you do copy an existing code consider why it did things the way it did, the same factors might not apply in your case or the old code was poorly written from the first place.

Open a Pull request
-------------------

When you are done, and the additions and modifications to the code are in place, commit your changes, and get your code reviewed by opening a new Pull Request. Please remember to follow the Pull Request template.

In the Pull Request template you will be required to add a "Test plan". For example, if you performed GUI changes, demonstrate it by posting a screenshot. Make a list of steps to be taken by the reviewers to verify that the changes are working as expected. This is also a good point to consider any potential edge-cases or different kinds of inputs if you didn't already do it while writing the code. Perform the steps you described when making the PR even if they seem trivial and you did them during development, it helps to catch any mistakes done during the final cleanup and making sure you didn't forget anything.


Repeat
-------

**Thank you!** You've made your very first contribution, and Cutter is better for it. But don't stop now. Go back to the first steps, as there is plenty more to do. A mentor or other developers might suggest a new issue for you to work on.


More Information
-----------------

.. important::
    We're always in the process of improving the information on this page for newcomers to the Cutter. Please help us by suggesting improvements and tell us about the information that this page lacks.

