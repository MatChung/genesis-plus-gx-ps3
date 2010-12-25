make.exe
make_fself_npdrm  Genesisplus.elf  ps3/GENP00001/USRDIR/EBOOT.BIN
python ps3/PS3py/sfo.py --title "GenesisPlus V1.1" --appid "GENP00001" -f ps3/PS3py/sfo.xml PS3/GENP00001/PARAM.SFO
python ps3/PS3py/pkg.py  --contentid IV0002-GENP00001_00-SAMPLE0000000001 ps3/GENP00001/ Genesisplus_Beta.pkg
pause
