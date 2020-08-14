Contributing to Cutter
=======================

So you like Cutter and want to get involved? Great! This part of the documentation will help and guide you through everything you need to know when contributing to Cutter. Welcome, we're delighted to see you!

.. tip::
   **Need help?** Our community strives to be friendly, open, and accessible for new contributors. If you have any difficulties getting involved or finding answers to you questions, please `come and ask your questions on our Telegram or IRC groups <https://cutter.re/#community>`_.

   We know that set up to work on Cutter and finding issues that are a good fit for your skills can be a challenge. We're always looking for ways to improve this process: making Cutter more open, accessible, and easier to participate with. If you're having any trouble following this documentation or hit a barrier you can't get around, please contact us.

.. rubric:: How do you want to help?

.. raw:: html

   <a class="btn btn-outline-primary btn-lg m-2" role="button" href="contributing/code/getting-started.html">Code</a>
   <a class="btn btn-outline-warning btn-lg m-2" role="button" href="contributing/docs/getting-started.html">Documentation</a>
   <a class="btn btn-outline-success btn-lg m-2" role="button" href="contributing/plugins/getting-started.html">Plugins</a>
   <a class="btn btn-outline-info btn-lg m-2" role="button" href="contributing/translations/getting-started.html">Translations</a>

steps for contributing
======================
- Have a good knowledge of git and github. we highly recomend to read the `GitHub-Contributing-to-a-Project <https://git-scm.com/book/en/v2/GitHub-Contributing-to-a-Project>`_.
- Fork the repository on GitHub.
- clone the repository.
    ``git clone --recurse-submodules https://github.com/radareorg/cutter``.
- add the fork to your local repository by
    ``git remote add fork https://github.com/<youride>/cutter``.
- Create a branch based on topic. avoid working on master.
    ``git checkout -b <branch name``.
- Make changes and commit by
    ``git add .``
    ``git commit -m "commit message"``
- Push it to your forked branch.
    ``git push fork``
- Create a Pull request and wait for review. If your commit solves any issues then specify those as "closes #ISSUE_NUM"
- If any changes requested then do the changes locally and commit changes and push it your fork.
- If everything is okay your request will be merged and repeat.
- If you are working on new feature until your PR gets merged then create another branch and work on it. Don't do anything on master.


---------

.. toctree::
   :maxdepth: 2
   :titlesonly:

   contributing/code
   Contributing Documentation <contributing/docs/getting-started>
   contributing/translations/getting-started
   contributing/plugins/getting-started
