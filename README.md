# cpp-cli

Implementation of a class that provide useful and better looking cli applications for C++. It claims the terminal buffer and lets you to update it any time you want.

- Dynamicly resizeable.
- It lets adding fixed positioned and updateable lines on the top of terminal screen.
- It lets appending logs.
- It lets saving logs to a file.
- It lets the user to send inputs as key or string. (default press 'i' to write as string (insert mode) and press 'esc' to go back the command mode)
- Provides "on_start", "on_close", etc. functionality up to your applications procedures.
- Provides blocking and unblocking interrupt signals sent by 'Ctrl + C' etc.
