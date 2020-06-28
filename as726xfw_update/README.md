# AS726x firmware upgrade

The chip I bought arrived with an old and buggy firmware (2.04). This
directory contain files extracted from the [AS72xx_EvalSW_v5-1-0.zip
AS72xx Firmware Updater](https://ams.com/as7261#tab/tools) that I used
do upgrade to the version 12.0, which works much better. Please note
that the command are **not** compatible with the old version of the
firmware. See the attached *xls* file and forget about what is in the
data sheet.

I used `tcl/tk 8.6.10` running on an `Arch Linux` system. The devices
were connected via a `FT232` 3.3V UART (select the appropriate
`/dev/ttyUSBx` device in the upgrade tool).
