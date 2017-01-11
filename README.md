# gchartman
Historic archive of one of my first large applications, migrated from SourceForge.
This repository corresponds to version 0.1.2, released on 26.2.2001.

## Screenshots

### Main Window / Stock Selection Window
The main window shows a categorized list of stocks. The data is saved in a MySQL database, to which you need to connect.
![Main Window / Stock Selection Window](/screenshots/ss_main.png?raw=true)

### Stock Chart Analysis Window
This is the UI for viewing and annotating charts of stock prices. You can put indicators on the main chart curve (moving average, RSI, momentum).
You can also stack indictors, to get eg. a moving average of an RSI indicator. You can also freely place trendlines, and annotate the chart with
comments and buy/sell indicators.
![Stock Chart Analysis Window](/screenshots/ss_0.1.2_emtv.png?raw=true)

### Videotext Data Import Configuration Window
Unmetered, always-on Internet not having been a thing yet, a main source for stock market data was Videotext (Teletext).
If you have a TV-capture card, you can set up a set of pages to capture and import into the database. For each page, you have to define
the areas in which the data you want to capture is printed.
![Videotext Data Import Configuration Window](/screenshots/ss_vt.png?raw=true)
