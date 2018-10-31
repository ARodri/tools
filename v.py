#!/usr/bin/env python
import argparse
import curses
import sys
import os
import csv

from collections import defaultdict
import math

#log = open("log.txt","w")

class PadError(Exception):
    pass

class Dim:
    def __init__(self,y1,x1,y2=-1,x2=-1,dy=1,dx=1):
        self.y1 = y1
        self.x1 = x1
        self.y2 = y2
        self.x2 = x2
#        self.dy = dy
        self.dx = dx


class Panel:
    def __init__(self,screen,win,scrDim, margin, defaultAttr = curses.A_NORMAL):
        self.screen = screen
        self.win = win
        self.scrDim = scrDim
        self.cornerRowIdx = 0
        self.cornerColIdx = 0
        self.margin = margin
        self.defaultAttr = defaultAttr

        height,width = self.win.getmaxyx()
        dx = scrDim.dx if scrDim.dx > 0 else width
        self.gridHeight = height
        self.gridWidth = int(width/dx) + (width % dx > 0)

        self.alignAtX = [dx * i for i in range(0, self.gridWidth)]
        self.innerWidths = [dx - self.margin] * self.gridWidth

    def validated(self,gridY,gridX):
        gridY2 = min(max(0,gridY),self.gridHeight-1)
        gridX2 = min(max(0,gridX),self.gridWidth-1)
        return gridY2,gridX2

    def move(self,gridY,gridX):
        self.cornerRowIdx,self.cornerColIdx = self.validated(gridY,gridX)

    def rmove(self,dGridY,dGridX):
        self.move(self.cornerRowIdx+dGridY,self.cornerColIdx+dGridX)

    def widen(self,colIdx, dx=1):
        newWidth = self.innerWidths[colIdx] + dx 
        if newWidth < 1:
            return False          
        self.innerWidths[colIdx] = newWidth

        for i in range(colIdx+1, self.gridWidth):
            self.alignAtX[i] += dx

        height,width = self.win.getmaxyx()
        self.win.resize(height,width+dx)

        return True


    @staticmethod
    def normalized(x,maxX, windingIndex=True):
        if x < 0:
            trueX = maxX + x if windingIndex else max(x,0)
        else:
            trueX = min(maxX-1,x)
        return trueX

    def getCorners(self):
        maxY,maxX = self.screen.getmaxyx()
        y1 = Panel.normalized(self.scrDim.y1, maxY)
        x1 = Panel.normalized(self.scrDim.x1, maxY)
        y2 = Panel.normalized(self.scrDim.y2, maxY)
        x2 = Panel.normalized(self.scrDim.x2, maxX)
        return (y1,x1,y2,x2)

    def noutrefresh(self):
        y1,x1,y2,x2 = self.getCorners()

        padY = self.cornerRowIdx
        padX = self.alignAtX[self.cornerColIdx]

        self.win.noutrefresh(padY, padX, y1,x1,y2,x2)

    def addnstr(self,*args):
        # force drawing
        try:
            self.win.addnstr(*args)
        except:
            pass

class Canvas:
    def __init__(self, header,rows, conf):
        self.header = header
        self.rows = rows
        self.conf = conf

        self.tabulate()

        self.numCol = len(header)
        self.numRow = len(rows)


        if self.numRow > 0:
            self.rowNumWidth = max(len(str(self.numRow)), len(self.conf["delim"])) + 1 # 1 for border
        else:
            self.rowNumWidth = 1

        self.welcome = True

        self.dialogs = {}

        self.displayRowIdxFrom = 0
        self.displayNumRow = conf["bufferedRows"]

        self.hlRowIdx = self.displayRowIdxFrom # real, not display idx
        self.hlColIdx = 0 # real, not display idx



    def tabulate(self):
        maxNumCol = max([len(row) for row in self.rows])
        self.misalignedAt = {}

        # fill header
        numCol = len(self.header)
        if maxNumCol > numCol:
            self.header += ["C{}".format(i) for i in range(numCol+1, maxNumCol+1)]
            self.misalignedAt[-1] = numCol

        # fill rows        
        for i in range(0,len(self.rows)):
            numCol = len(self.rows[i])
            if maxNumCol > numCol:
                self.rows[i] += ["?"]*(maxNumCol - numCol)
                self.misalignedAt[i] = numCol
        

    def widen(self,colIdx,dx=1):
        success = self.headerPanel.widen(colIdx,dx)
        if not success:
            return False

        success = self.mainPanel.widen(colIdx,dx)
        if not success:
            return False

        self.drawHeader()
        self.drawMain()
        self.drawHighlight()

        return True

    def createPanel(self,height, width, margin,y1,x1,y2,x2,dy=1,dx=-1,attr=curses.A_NORMAL):
        try:
            win = curses.newpad(height,width)
        except:
            raise PadError()

        panel = Panel(self.screen,win,Dim(y1,x1,y2,x2,dy,dx),margin, attr)
        return panel


    def drawHeader(self):
        panel = self.headerPanel
        panel.win.clear()
        alignAtX = panel.alignAtX
        for i in range(0,self.numCol):
            text = ("{:<" + str(panel.innerWidths[i]) + "}").format(self.header[i])
            panel.addnstr(0, alignAtX[i], text, panel.innerWidths[i],panel.defaultAttr)
            if len(self.header[i]) > panel.innerWidths[i]:
                panel.addnstr(0,alignAtX[i]+panel.innerWidths[i],'>', 1, Canvas.cyan)

        if -1 in self.misalignedAt:
            for i in range(self.misalignedAt[-1],self.numCol):
                panel.win.chgat(0,alignAtX[i], panel.innerWidths[i],
                                panel.defaultAttr | Canvas.color4Misaligned)

        
    def drawMain(self):
        panel = self.mainPanel
        panel.win.clear()
        alignAtX = panel.alignAtX
        mainAttr = panel.defaultAttr

        for displayY in range(0,self.displayNumRow):
            y = displayY + self.displayRowIdxFrom  #shift#
            if y >= self.numRow:
                continue
            for i in range(0,self.numCol):
                panel.addnstr(displayY,alignAtX[i], self.rows[y][i], panel.innerWidths[i], mainAttr)

                if len(self.rows[y][i]) > panel.innerWidths[i]:
                    panel.addnstr(displayY,alignAtX[i]+panel.innerWidths[i],'>',1,Canvas.cyan)  #shift#

        for y in [rowIdx for rowIdx in self.misalignedAt.keys() \
                  if rowIdx >= 0 and rowIdx >= self.displayRowIdxFrom]:
            displayY = y - self.displayRowIdxFrom  #shift#
            for i in range(self.misalignedAt[y],self.numCol):
                panel.win.chgat(displayY,alignAtX[i], panel.innerWidths[i],
                                mainAttr | Canvas.color4Misaligned)


    def drawRowNum(self):
        panel = self.rowNumPanel
        height,width = panel.win.getmaxyx()
        for displayY in range(0, min(self.displayNumRow, self.numRow - self.displayRowIdxFrom)):
            rowLabel = ("{:<" + str(self.rowNumWidth-1) + "}").format(displayY + self.displayRowIdxFrom +1)
            panel.addnstr(displayY,0,rowLabel,-1)
        panel.win.vline(0,width-1,curses.ACS_VLINE,height)


    def drawDelim(self):
        self.delimPanel.addnstr(0,0,self.conf["delim"],-1,self.delimPanel.defaultAttr)

    def drawInfo(self):
        panel = self.infoPanel
        height,width = panel.win.getmaxyx()

        info1 = "Keys: arrows, PageUp, PageDown, a, e, t, b, [, ], s, q."
        attr = self.infoPanel.defaultAttr       
        panel.addnstr(1,0,info1, -1, attr)
        
        if len(info1) < width:
            info2 = " s (summary) may be slow for large files."
            attr = Canvas.red
            panel.addnstr(1,len(info1),info2,-1,attr)
            

        panel.win.hline(0,0,curses.ACS_HLINE,width, Canvas.blue)

    def drawValue(self):
        panel = self.valuePanel
        height,width = panel.win.getmaxyx()

        if self.welcome:
            panel.addnstr(0,0,self.conf["greetings"],-1,Canvas.red)
        else:
            rc1 = "[R{},C{}:".format(self.hlRowIdx+1, self.hlColIdx+1)
            rc2 = self.header[self.hlColIdx]

            rc3 = "] "
            rc = rc1 + rc2 + rc3
            text = self.rows[self.hlRowIdx][self.hlColIdx]

            filler = ' ' * max(0, width - len(rc) - len(text))
            fullText = rc + text + filler
            panel.addnstr(0,0,fullText,-1, self.valuePanel.defaultAttr)
            panel.win.chgat(0,len(rc1),len(rc2), self.valuePanel.defaultAttr | curses.A_BOLD)

        panel.win.hline(1,0,curses.ACS_HLINE,width, Canvas.blue)


    # generate summary information for display in dialog box.
    def __summarize(self):
        summary = {'numerical':True,
                   'min': None,
                   'max': None,
                   'mean': None,
                   'non-empty':0,
                   'misaligned':0,
                   'empty':0,
                   'freq':defaultdict(int)
               }
        
        # pretend all values are numerical
        #numbers = []
        sumnum = 0.0
        numnum = 0
        for rowIdx in range(0,len(self.rows)):
            row = self.rows[rowIdx]
            value = row[self.hlColIdx]
            misaligned = rowIdx in self.misalignedAt and self.misalignedAt[rowIdx] <= self.hlColIdx
            if misaligned:
                summary['misaligned'] += 1
                continue
            elif not value:
                summary["empty"] += 1
                continue
            summary["non-empty"] += 1

            summary['freq'][value] += 1
            
            # if len(summary['freq']) > self.conf['topk']:
            #     minEntry,minCount = None,0
            #     for entry,count in summary['freq'].items():
            #         if minEntry == None or minCount > count:
            #             minEntry,minCount = entry,count
            #     del summary['freq'][minEntry]

            if not summary['numerical']:
                continue

            try:
                numeric = float(value)
                #numbers.append(numeric)
                sumnum += numeric
                numnum += 1
                if summary['min'] == None or numeric < summary['min']:
                    summary['min'] = numeric
                if summary['max'] == None or numeric > summary['max']:
                    summary['max'] = numeric
            except:
                summary['numerical'] = False
                

        if summary['numerical'] and numnum > 0:
            summary['mean'] = sumnum/numnum

        return summary

    def drawDialog(self):
        if self.hlColIdx in self.dialogs:
            self.dialogPanel = self.dialogs[self.hlColIdx]
            return

        summary = self.__summarize()
        summaryText = []

        for key in ['non-empty',"empty"]:
            summaryText.append("{:<10}: ".format(key) + str(summary[key]))
        if summary['misaligned'] > 0:
            summaryText.append("{:<10}: ".format("misaligned") + str(summary["misaligned"]))

        if summary['numerical']:
            summaryText.append('')
            for key in ['min','max','mean']:
                summaryText.append("{:<10}: {}".format(key, summary[key]))

        if len(summary["freq"]) > 0:
            summaryText.append('')
            sortedKV = sorted(summary["freq"].items(), key = lambda pair: pair[1], reverse=True)
            numWidth = len(str(sortedKV[0][1]))
            for i in range(0,min(self.conf['topk'],len(sortedKV))):
                summaryText.append(("{:<"+str(numWidth)+"} '{}'").format(sortedKV[i][1],sortedKV[i][0]))

        height,width = self.screen.getmaxyx()
        cornerY, cornerX = 1,self.rowNumWidth
        dialogHeight = min(len(summaryText)+2,height - cornerY)
        dialogWidth = min(max([len(s) for s in summaryText])+2,width-cornerX)
        self.dialogPanel = self.createPanel(
            dialogHeight, dialogWidth,
            0,
            cornerY,cornerX,dialogHeight,self.rowNumWidth + dialogWidth)

        panel = self.dialogPanel

        for i in range(0,len(summaryText)):
            panel.addnstr(i+1,1,summaryText[i],dialogWidth-2)
            
        panel.win.attron(Canvas.yellow)
        panel.win.box()

        self.dialogs[self.hlColIdx] = panel

    def drawHighlight(self, restore=False):

        if restore:
            mainAttr = self.mainPanel.defaultAttr
            headerAttr = self.headerPanel.defaultAttr
            rowNumAttr = self.rowNumPanel.defaultAttr
        else:
            mainAttr = self.mainPanel.defaultAttr | curses.A_REVERSE
            headerAttr = self.headerPanel.defaultAttr | Canvas.whiteOnMagenta
            rowNumAttr = self.rowNumPanel.defaultAttr | Canvas.whiteOnMagenta

        if self.hlRowIdx in self.misalignedAt and self.hlColIdx >= self.misalignedAt[self.hlRowIdx]:
            mainAttr |= Canvas.color4Misaligned

        if -1 in self.misalignedAt and self.hlColIdx >= self.misalignedAt[-1]:
            headerAttr |= Canvas.color4Misaligned
        
        # modify main panel
        padY = self.hlRowIdx - self.displayRowIdxFrom #shift#

        padX = self.mainPanel.alignAtX[self.hlColIdx]
        innerWidth = self.mainPanel.innerWidths[self.hlColIdx]
        self.mainPanel.win.chgat(padY,padX,innerWidth, mainAttr)

        # modify header panel
        padY = 0
        padX = self.headerPanel.alignAtX[self.hlColIdx]
        self.headerPanel.win.chgat(padY,padX,innerWidth, headerAttr)
        
        # modify row num
        padY = self.hlRowIdx - self.displayRowIdxFrom #shift#
        padX = self.rowNumPanel.alignAtX[0]
        innerWidth = self.rowNumPanel.innerWidths[0]
        self.rowNumPanel.win.chgat(padY,padX,innerWidth, rowNumAttr)

    def rmove(self,dy,dx):
        self.mainPanel.rmove(dy,dx)
        self.headerPanel.rmove(0,dx)
        self.rowNumPanel.rmove(dy,0)

    def hlrmove(self,dy=0,dx=0, windingIndex = True):
        self.hlmove(self.hlRowIdx+dy,self.hlColIdx+dx, windingIndex)

    def hlmove(self,y=None,x=None, windingIndex = True):
        if y == None:
            y = self.hlRowIdx
        if x == None:
            x = self.hlColIdx

        if y>=self.numRow:
            y = self.numRow - 1
        if y<0:
            y = self.numRow + y if windingIndex else 0

        repaged = False
        if y < self.displayRowIdxFrom \
           or y >= self.displayRowIdxFrom + self.displayNumRow:
            halfDisplay = int(self.displayNumRow/2)
            if y + halfDisplay < self.numRow:
                self.displayRowIdxFrom = max(0, y - halfDisplay)
            else:
                self.displayRowIdxFrom = max(0, self.numRow - self.displayNumRow)
            repaged = True

        if repaged:
            self.drawRowNum()
            self.drawMain()
            
        if self.hlRowIdx != y or self.hlColIdx != x:
            if not repaged:
                self.drawHighlight(restore=True)

            displayY = -1 if y < 0 else y - self.displayRowIdxFrom
            self.hlRowIdx = Panel.normalized(displayY,self.numRow, windingIndex) \
                            + self.displayRowIdxFrom
            #self.hlColIdx = Panel.normalized(x,self.mainPanel.gridWidth, windingIndex)
            self.hlColIdx = Panel.normalized(x,self.numCol, windingIndex)


        self.drawHighlight(restore=False)

        # test if out of screen 
        maxY,maxX = self.screen.getmaxyx()

        panel = self.mainPanel

        while True:
            y1 = Panel.normalized(panel.scrDim.y1, maxY)
            y2 = Panel.normalized(panel.scrDim.y2, maxY)
            hlFromCornerY = y1 + (self.hlRowIdx - self.displayRowIdxFrom) - panel.cornerRowIdx
            if hlFromCornerY < y1:
                self.rmove(-1,0)
            elif hlFromCornerY > y2:
                self.rmove(1,0)
            else:
                break

            
        while True:
            x1 = Panel.normalized(panel.scrDim.x1, maxY)
            x2 = Panel.normalized(panel.scrDim.x2, maxX)
            if self.hlColIdx == len(panel.alignAtX) - 1:
                rightBorder = panel.win.getmaxyx()[1] - 1
            else:
                rightBorder = panel.alignAtX[self.hlColIdx+1] - 1
            hlFromCornerX = x1 + rightBorder - panel.alignAtX[panel.cornerColIdx]
            if hlFromCornerX < x1:
                self.rmove(0,-1)
            elif hlFromCornerX > x2 and self.hlColIdx > panel.cornerColIdx:
                self.rmove(0,1)
            else:
                break
            
    def trim(self):
        panel = self.mainPanel
        y1,x1,y2,x2 = panel.getCorners()

        height,width = panel.win.getmaxyx()
        fromCornerY = y1 + (height-1) - panel.cornerRowIdx
        fromCornerX = x1 + (width-1) - panel.alignAtX[panel.cornerColIdx]
        
        if fromCornerY < y2:
            for x in range(0,x2+1):
                self.screen.move(fromCornerY+1,x)
                self.screen.clrtobot()
            
        if fromCornerX < x2:
            y0 = self.headerPanel.scrDim.y1
            for y in range(y0,y2+1):
                self.screen.move(y,fromCornerX+1)
                self.screen.clrtoeol()


    def refresh(self, showDialog=False):

        # # causing flickery
        # self.screen.clear()        
        self.trim()

        self.screen.noutrefresh()
        self.headerPanel.noutrefresh()
        self.mainPanel.noutrefresh()
        self.rowNumPanel.noutrefresh()
        self.delimPanel.noutrefresh()
        self.infoPanel.noutrefresh()
        self.valuePanel.noutrefresh()
        self.screen.noutrefresh()

        if showDialog:
            self.dialogPanel.noutrefresh()

        curses.doupdate()

    def show(self, screen):
        # this is the entry point

        self.screen = screen

        curses.use_default_colors()
        curses.init_pair(1, curses.COLOR_RED, -1)
        curses.init_pair(2, curses.COLOR_GREEN,-1)
        curses.init_pair(3, curses.COLOR_BLUE, -1)
        curses.init_pair(4, curses.COLOR_MAGENTA, -1)
        curses.init_pair(5, curses.COLOR_CYAN, -1)
        curses.init_pair(6, curses.COLOR_YELLOW, -1)
        curses.init_pair(7, curses.COLOR_WHITE, curses.COLOR_BLUE)
        curses.init_pair(8, curses.COLOR_WHITE, curses.COLOR_MAGENTA)

        Canvas.red = curses.color_pair(1)
        Canvas.green = curses.color_pair(2)
        Canvas.blue = curses.color_pair(3)
        Canvas.magenta = curses.color_pair(4)
        Canvas.cyan = curses.color_pair(5)
        Canvas.yellow = curses.color_pair(6)
        Canvas.whiteOnBlue = curses.color_pair(7)
        Canvas.whiteOnMagenta = curses.color_pair(8)
        Canvas.color4Misaligned = Canvas.red

        dataWidth = max(100,self.numCol * self.conf["cellWidth"])
        #dataWidth = self.numCol * self.conf["cellWidth"]
        self.headerPanel = self.createPanel(
            1, dataWidth,
            self.conf["margin"],
            2,self.rowNumWidth,2,-1,
            1,self.conf["cellWidth"],
            curses.A_UNDERLINE | curses.A_BOLD)

        self.mainPanel = self.createPanel(
            self.displayNumRow, dataWidth,
            self.conf["margin"],
            3, self.rowNumWidth, -3,-1,
            1,self.conf["cellWidth"])


        self.rowNumPanel = self.createPanel(
            self.displayNumRow, self.rowNumWidth,
            1,
            3,0,-3,self.rowNumWidth)


        self.delimPanel = self.createPanel(
            1,self.rowNumWidth-1,
            1,
            2,0,2,self.rowNumWidth,
            1,-1,
            Canvas.cyan | curses.A_BOLD)

        self.infoPanel = self.createPanel(
            2, dataWidth,
            0,
            -2,0,-1,-1,
            1,-1,
            Canvas.green)

        self.valuePanel = self.createPanel(
            2, dataWidth,
            0,
            0,0,1,-1)

        self.dialogPanel = self.createPanel(
            5, 20,
            0,
            0,0,5,20)

        self.drawHeader()
        self.drawMain()
        self.drawRowNum()
        self.drawDelim()
        self.drawInfo()
        self.drawValue()
        self.drawDialog()

        self.hlmove(self.displayRowIdxFrom,0)

        try:
            self.refresh() # may fail if terminal too small
        except:
            pass

        curses.curs_set(0)

        dialogShown = False
        while True:

            curses.flushinp()
            c = self.screen.getch()

            if c == ord('q') and not dialogShown:
                break
                
            showDialog = False
            redraw = True
            halfPage = int((self.mainPanel.getCorners()[3]+1)/2)

            if c == curses.KEY_DOWN:
                self.hlrmove(1,0)
            elif c == curses.KEY_UP:
                self.hlrmove(-1,0, windingIndex=False)
            elif c== curses.KEY_LEFT:
                self.hlrmove(0,-1, windingIndex=False)
            elif c== curses.KEY_RIGHT:
                self.hlrmove(0,1)
            elif c == curses.KEY_HOME or c == ord('a'):
                self.hlmove(x=0)
            elif c == curses.KEY_END or c == ord('e'):
                self.hlmove(x=-1)
            elif c == ord('t'):
                self.hlmove(y=0)
            elif c == ord('b'):
                self.hlmove(y=-1)
            elif c == curses.KEY_PPAGE:
                self.hlrmove(dy=-halfPage, windingIndex=False)
            elif c == curses.KEY_NPAGE:
                self.hlrmove(dy=halfPage, windingIndex=False)
            elif c == ord('['):
                self.widen(self.hlColIdx,-2)
            elif c == ord(']'):
                self.widen(self.hlColIdx,2)
            elif c == ord('s') and not dialogShown:
                self.drawDialog()
                showDialog = True
            elif c == curses.KEY_RESIZE:
                redraw = True
            else:
                redraw = dialogShown

            if redraw:
                if self.welcome:
                    self.welcome = False
                    self.drawInfo()

                self.drawValue()

                try:
                    self.refresh(showDialog)
                    dialogShown = showDialog
                except:
                    pass


class Automator:
    @staticmethod
    def delimeter(line):
        dlmts = [',', '|', ' ', '\t']
        numCols = [(d,line.count(d)+1) for d in dlmts]
        numCols.sort(key=lambda pair: pair[1], reverse=True)
        return numCols[0]

    @staticmethod
    def open(fileName):
        if fileName:
            f = open(fileName)
        else:
            f = sys.stdin
        return f

    @staticmethod
    def giveBackTTY():
        new_stdin = open("/dev/tty")
        if new_stdin.fileno() != 0:
            os.dup2(new_stdin.fileno(),0)
        new_stdin.close()
        sys.stdin = os.fdopen(0)

        
######################################################
if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        description="Terminal viewer for delimited files. By Shengjun Pan @2013.",
        epilog="Keys:\n"
        + "  Arrows     move to an ajacent cell.\n"
        + "  Page Up    move up by half screen.\n"
        + "  Page Down  move down by half screen.\n"
        + "  HOME, a    move to first column in current row.\n"
        + "  END, e     move to last column in current row.\n"
        + "  t          move to top row in current column.\n"
        + "  b          move to bottom row in current column.\n"
        + "  [          shrink current column.\n"
        + "  ]          widen current column.\n"
        + "  s          show or hide (if on) summary on current column.\n"
        + "  q          hide summary (if on) or quit.\n"
        + "  (other)    any other key hides the summary (if on).\n",
        formatter_class=argparse.RawDescriptionHelpFormatter)

    parser.add_argument(dest='inFile', nargs='?', metavar='FILE',
                        help="Delimited file. If not given, read from standard input.")
    parser.add_argument('-d', '--delimiter', dest='delim',
                        help="delimiter. It is used to directly split the input;"
                        + "quotes are not handled like in csv.")
    parser.add_argument('-w', '--width', dest='colWidth', default=20, type=int,
                        help="width of a cell, not including margin (fixed to 3). Default:20.")
    parser.add_argument('-n', '--num-rows', dest='numRows', default=1000,type=int,
                        help="Read at most NUMROWS many rows."
                        + " If not positive, all rows are read."
                        + " Default:1000.")
    parser.add_argument('-k', '--topk', dest='topk', default=10,type=int,
                        help="Number of highest frequencies to be displayed."
                        + " Default:10.")
    parser.add_argument('-r', '--headerless', dest='noHeader', action='store_true', default=False,
                        help="If given, an automatic header 'C1,C2,...' is used.")

    args = parser.parse_args(sys.argv[1:])
    locals().update(vars(args))

    print >>sys.stderr, "Loading file... [Ctr+C to interrupt]"

    # read first line
    f = Automator.open(inFile)

    firstLine = f.readline()
    if not firstLine:
        print >>sys.stderr, "Empty input."
        exit(1)

    firstLine = firstLine.rstrip("\n\r")
       
    # determine delimiter
    if delim == None:
        delim,numCol = Automator.delimeter(firstLine)
    else:
        delim = delim.decode("string-escape")
        numCol = firstLine.count(delim)

    # read rows
    if noHeader:
        header = ["C{}".format(i) for i in range(1,numCol+1)]
        data = [firstLine.split(delim)]
        count = 1
    else:
        header = firstLine.split(delim)
        data = []
        count = 0


    reader = csv.reader(f, delimiter=delim)
    for row in reader:
        try:
            count += 1

            data.append(row)
            
            if numRows >0 and count >= numRows:
                count = -count
                break

            if count % 1000000 == 0:
                print>>sys.stderr, " ", count, " lines [Ctr+C to interrupt]"

        except KeyboardInterrupt:
            count = -count
            break


    if len(data) > abs(count): # corrupted
        del data[-1]

    if not data:
        print >>sys.stderr, "Only header, no data. Use hdr.py instead."
        exit(1)

    print >>sys.stderr, "Data loaded. Do not interrupt now."

    if f == sys.stdin:
        Automator.giveBackTTY()

    if count < 0:
        greetings = "Warning: only showing first " + str(-count) + " rows. Use option '-n0' to load all."
    else:
        greetings = "Read " + str(count) + " rows"

    # set parameters
    margin = 3
    if topk <= 0:
        topk = 1
    conf = {"delim": delim.encode("string-escape"),
            "margin": margin,
            "cellWidth": max(margin+1,colWidth),
            "greetings": greetings,
            "topk": topk,
            "bufferedRows":1000}

    canvas = Canvas(header,data,conf)

    try:
        curses.wrapper(canvas.show)
    except PadError:
        print >> sys.stderr, "Failed to create pad;" \
            + "data may be too large. " \
            + "Consider displaying fewer rows with option option '-n'."
