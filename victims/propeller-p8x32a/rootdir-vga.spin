CON
    _clkmode = xtal1 + pll16x
    _xinfreq = 5_000_000

OBJ
    sd : "SD-MMC_FATEngine"
    term : "vga_text"

PUB main | err   
   term.start(16)
   repeat
        err := \experiment
        if err
            term.Str(err)
        else
            term.Str(string("Ok.", $D))
        waitcnt(clkfreq / 5 + cnt)
        
pri experiment | entryName
    term.out($00)

    term.Str(string("Root directory:", $D))
 
    if !sd.FATEngineStart(0, 1, 2, 3, -1, -1, -1, -1, -1)
        term.Str(string("Failed to start", $D))
        return
 
    sd.mountPartition(0)

    sd.listEntries("W")
    repeat while (entryName := sd.listEntries("N"))
        term.Str(string("file ["))
        term.Str(entryName)
        term.Str(string("]", $D))

    sd.unmountPartition
 