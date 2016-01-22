# Search+

>NOTE: *The downloadbale binary file of the plugin was not copied when I migrated the project from Google code to github. For now, you're gonna have to checkout the code and build it yourself. I will add downloadable binaries when I get my hands on VS compiler.*

Search+ is a notepad++ plugin that lets you search a file for multiple keywords in a single shot. You can specify a list of keywords to search for in the current file, and filter out lines that match any of the keywords in this list. It was developed mainly for analyzing large log files where you are interested in more than one keyword and the order in which they appear.

* Matching lines are listed with their line numbers in a separate panel in the plugin window
* Double clicking a matched line in this panel will take you to the corresponding line in the original document
* Options to copy the filtered lines to clipboard and highlight matches in the original file
* Supports case sensitive search, whole word matchng and regular expressions. Regexp is enabled by default

In case of any bugs or suggestions, please raise an issue or leave a <a href='http://amarghosh.blogspot.com/2012/08/searchplus.html'>comment here</a>.

# Installation

* Extract `SearchPlus.dll` from the zip file
* Close notepad++ if it is currently running
* Copy the dll to `<Notepad++ Installation path>\plugins` folder. This is typically `C:\Program Files\Notepad++\plugins`. You might need admin privileges if the installation path is inside `C:\Program Files`

# Usage

 * Start notepad++ and press `Alt+Q` for launching the plugin window (alternatively, you can select `Search+` from the `plugins` menu)
 * Type the required search keywords in the input field, mark the options (case sensitivity, whole word search, plain text) and _Add_ them to the keyword list (top right).
 * The selected word (or the word under cursor if there is no selection) from the current tab of notepad++ will be populated in the input field by default
 * Once the required keywords are added to the list, click _Search_ to search for these keywords in the current file
 * Matching lines will be listed in the main output area.
 * You can `Copy` the results to clipboard or `Highlight` the matches in the original document
 * Clicking a pattern from the list will update the input field and checkboxes to reflect its values. Double click the pattern to edit it.
  * Closing the plugin window by clicking window close button will clear the search results. The keyword list will be preserved even after closing notepad++
  * You can hit `Escape` to hide the window without losing the search results. Press `Alt+Q` again to bring it back

# Other features and known issues
 * Default button (the one mapped to the Enter key when focus is on the input field) is _Add_ during normal mode and _Update_ during edit.
