CON
    _clkmode = xtal1 + pll16x
    _xinfreq = 5_000_000

OBJ
    term : "Parallax Serial Terminal"
    sd : "SD-MMC_FATEngine"

PUB main | err
    term.Start(115200)
    repeat
        err := \experiment
        if err
            term.Str(err)
        else
            term.Str(string("Ok.", term#NL))
        waitcnt(clkfreq / 5 + cnt)
        
pri experiment | entryName
    term.Clear
    term.Str(string("Root directory:", term#NL))
 
    if !sd.FATEngineStart(0, 1, 2, 3, -1, -1, -1, -1, -1)
        term.Str(string("Failed to start", term#NL))
        return
 
    sd.mountPartition(0)
    
    sd.listEntries("W")
    repeat while (entryName := sd.listEntries("N"))
        term.Str(string("file ["))
        term.Str(entryName)
        term.Str(string("]", term#NL))

    sd.unmountPartition
 