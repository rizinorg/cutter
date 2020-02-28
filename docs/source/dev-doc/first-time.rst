First time contributing
=======================


* Start by reading :doc:`../dev-doc`
* See :doc:`../building` for compilation instructions
* Get the build working
* Run cutter and get familiar with how the functionality you want to modify works currently
* Find the the source files implementing it (practice searching code in your editor, this is an important skill)
* Come up with a plan for things that need to be done and discuss it in GitH	ub issue. Just because something was mentioned in issue description doesn't necessarily means that it is still relevant or good feature.
* Check if any of the existing code implements similar behavior in the same widget or similar widgets. If you do copy an existing code consider why it did things the way it did, the same factors might not apply in your case or the old code is simply bad. Still it's a way of moving forwards if you otherwise wouldn't know how to approach it.
* When code changes are done and you are ready for PR, please follow the PR template.

"Test plan (required)" really means that. If those are GUI changes make a screenshot.
Make a basic list of steps for checking that changes are working as expected. This also a good point to consider any potential edge cases or different kind of interesting inputs if you didn't already do it while writing code. Actually perform the steps you described when making the PR even if they seem trivial and you did them during development, it helps catching any makes done during final cleanup and making sure you didn't forget anything.
