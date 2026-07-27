class myError(Exception): pass

N_CELLS_DRS4 = 1024            # celle del buffer circolare DRS4 (V1742)
PEDESTAL_CALIB_EVENTS = 1000   # eventi di default per calibrare il piedistallo per-capacitor (correzione PADME)

_current_pedestal = None       # cache: {canale: np.array(N_CELLS_DRS4)} costruita dall'ultima open_mid()

class dgtz_header:      # very simple class for the dgtz header
    def __init__(self, a):
        self.ntriggers           = a[0]
        self.nchannels           = a[1]
        self.nsamples            = a[2]
        self.vertical_resulution = a[3]
        self.sampling_rate       = a[4]
        self.offsets             = a[5]
        self.TTT                 = a[6]
        self.SIC                 = a[7]
        if len(a)>8:
            self.nBoards         = a[8]
            self.boardNames      = a[9]
        else:
            self.nBoards        = 1
            self.boardNames     = [1742]
            print('WARNING: You are using an older version of the data bank. The analysis of the digitizers might be incomplete.')

        self.itemDict = {}
        self.itemDict["0"] = self.ntriggers
        self.itemDict["1"] = self.nchannels
        self.itemDict["2"] = self.nsamples
        self.itemDict["3"] = self.vertical_resulution
        self.itemDict["4"] = self.sampling_rate
        self.itemDict["5"] = self.offsets
        self.itemDict["6"] = self.TTT
        self.itemDict["7"] = self.SIC
        self.itemDict["8"] = self.nBoards
        self.itemDict["9"] = self.boardNames
    
    def __getitem__(self, index):
        return self.itemDict[str(int(index))]


def daq_dgz_full2header(bank, verbose=False):
    # v0.1 full PMT recostruction
    import numpy as np
    nboard              = bank.data[0]
    full_buffer_size    = len(bank.data)
    name_board          = np.empty([nboard], dtype=int)
    number_samples      = np.empty([nboard], dtype=int)
    number_channels     = np.empty([nboard], dtype=int)
    number_events       = np.empty([nboard], dtype=int)
    vertical_resulution = np.empty([nboard], dtype=int)
    sampling_rate       = np.empty([nboard], dtype=int)
    channels_offset     = []
    channels_ttt        = []
    channels_SIC        = []
    if verbose: print("Number of board: {:d}".format(nboard))
    ich=0
    for iboard in range(nboard): ######### cicle over the boards
        ich+=1
        name_board[iboard]          = bank.data[ich]
        ich+=1  
        number_samples[iboard]      = bank.data[ich]
        ich+=1  
        number_channels[iboard]     = bank.data[ich]
        ich+=1  
        number_events[iboard]       = bank.data[ich]
        ich+=1  
        vertical_resulution[iboard] = bank.data[ich]
        ich+=1  
        sampling_rate[iboard]       = bank.data[ich]

        if verbose:
            print ("board: {:d}, name_board: {:d}, number_samples: {:d}, number_channels: {:d}, number_events: {:d}, vertical_resulution: {:d}, sampling_rate: {:d}".format( 
                   iboard, name_board[iboard], number_samples[iboard], number_channels[iboard], number_events[iboard], vertical_resulution[iboard], sampling_rate[iboard]))
        
        ######### Channels offset reading:
        channels_offset_tmp = np.empty([number_channels[iboard]], dtype=int)
        for ichannels in range(number_channels[iboard]):
            ich+=1
            channels_offset_tmp[ichannels] = bank.data[ich]
        if verbose:
            print ("channels_offset: ", channels_offset_tmp, flush=True)
        channels_offset.append(channels_offset_tmp)
        
        ######### TTT reading:
        channels_ttt_tmp = np.empty(number_events[iboard], dtype=int)
        for ttt in range(number_events[iboard]):
            ich+=1
            channels_ttt_tmp[ttt] = bank.data[ich]
        if verbose:
            print ("channels_ttt: ", channels_ttt)
        channels_ttt.append(channels_ttt_tmp)
        
        ######### Start Index Cell reading:  
        if name_board[iboard] == 1742:   
            channels_SIC_tmp = np.empty(number_events[iboard], dtype=int)
            for sic in range(number_events[iboard]):
                ich+=1
                channels_SIC_tmp[sic] = bank.data[ich]
            channels_SIC.append(channels_SIC_tmp)
                
    full_header = dgtz_header([number_events, number_channels, number_samples, vertical_resulution, 
                              sampling_rate, channels_offset, channels_ttt, channels_SIC, nboard, name_board])
    return full_header


def correct_waveforms(waveform, SIC, number_channels, number_events, number_samples):
    # Correzione piedistallo per-capacitor (metodo PADME): S_cor(t) = S_meas(t) - E_p[(t+SIC) mod N_CELLS_DRS4]
    # Usa la calibrazione corrente (vedi open_mid / _build_pedestal_calibration). Se un canale non
    # e' presente nella calibrazione (es. calibrazione con meno canali del previsto) resta grezzo.
    import numpy as np

    corrected = []
    for ievent in range(number_events):
        sic = int(SIC[ievent])
        cell_idx = (np.arange(number_samples) + sic) % N_CELLS_DRS4
        for ich in range(number_channels):
            wf = np.asarray(waveform[ievent * number_channels + ich], dtype=float)
            E_p_ch = _current_pedestal.get(ich) if _current_pedestal is not None else None
            if E_p_ch is not None:
                wf = wf - E_p_ch[cell_idx]
            corrected.append(wf)
    return corrected


def daq_dgz_full2array(bank, header, verbose=False, ch_offset=[], pedestal_correction=True):

    if verbose: print("There are {} boards, and they are {}".format(header.nBoards, header.boardNames))

    data_offset = 0
    waveform = []

    for idigi,digitizer in enumerate(header.boardNames):

        ## Acquiring the "fast digitizer" data
        if str(digitizer) == '1742':

            number_events   = header[0][idigi]
            number_channels = header[1][idigi]
            number_samples  = header[2][idigi]
            SIC = header.SIC[idigi]

            for ievent in range(number_events):
                for ichannels in range(number_channels):
                    if verbose:
                        print ("data_offset, data_offset+number_samples",
                            data_offset, data_offset+number_samples)
                        print(bank.data[data_offset:data_offset+number_samples])

                    waveform.append(bank.data[data_offset:data_offset+number_samples])
                    data_offset += number_samples

            if pedestal_correction and _current_pedestal is not None:   ## Correzione piedistallo per-capacitor (PADME), se calibrata da open_mid()
                waveform = correct_waveforms(waveform, SIC, number_channels, number_events, number_samples)

            if verbose:
                print(number_channels, number_events, number_channels)

        else:
            raise myError("You seem to be trying to use a new digitizer model. You need to update the cygno libs for that.")

    return waveform

def mid_file(run, tag='WC', cloud=False, verbose=False):
    if cloud:
        BASE_URL  = BUCKET_REST_PATH+'cygno-data/'
        f = BASE_URL+(tag+'/run%05d.mid.gz' % run)
    else:
        f = ('/run%05d.mid.gz' % run)
    print(f)
    return f

def _build_pedestal_calibration(run, path, cloud, tag, verbose, n_calib):
    # Calibra il piedistallo per-capacitor (metodo PADME, mediana per cella fisica) sui primi
    # n_calib eventi del run, per TUTTI i canali presenti nei dati. Ritorna un dict
    # {canale: np.array(N_CELLS_DRS4)} in ADC counts, o None se non ci sono eventi disponibili.
    import numpy as np
    import midas.file_reader
    import os

    global _current_pedestal
    _current_pedestal = None   # durante la calibrazione si legge sempre il dato grezzo (niente auto-correzione)

    fname = mid_file(run, tag=tag, cloud=cloud, verbose=verbose)
    if not cloud:
        if not os.path.exists(path+tag+fname):
            raise myError("openFileError: "+path+tag+fname+" do not exist")
        f = midas.file_reader.MidasFile(path+tag+fname)
    else:
        filetmp = cmd.cache_file(fname, cachedir=path, verbose=verbose)
        f = midas.file_reader.MidasFile(filetmp)

    cal_matrix = None
    n_ev = 0
    for event in f:
        if n_ev >= n_calib:
            break
        for bank_name, bank in event.banks.items():
            if 'DGH0' not in bank_name:
                continue
            header = daq_dgz_full2header(bank, verbose=False)
            waveform = daq_dgz_full2array(event.banks['DIG0'], header, verbose=False)
            n_ch  = int(header.nchannels[0])
            n_sub = int(header.ntriggers[0])
            if cal_matrix is None:
                cal_matrix = {ch: np.empty((N_CELLS_DRS4, n_calib), dtype=np.int16) for ch in range(n_ch)}
            for isub in range(n_sub):
                if n_ev >= n_calib:
                    break
                sic = int(header.SIC[0][isub])
                spc = len(waveform[isub * n_ch])
                cell_idx = (np.arange(spc) + sic) % N_CELLS_DRS4
                for ch in range(n_ch):
                    wf = np.asarray(waveform[isub * n_ch + ch], dtype=np.int16)
                    cal_matrix[ch][cell_idx, n_ev] = wf
                n_ev += 1
            break

    if cal_matrix is None or n_ev == 0:
        if verbose:
            print("[wclib] Calibrazione piedistallo: nessun evento disponibile, correzione disattivata.")
        return None

    if n_ev < n_calib:
        for ch in cal_matrix:
            cal_matrix[ch] = cal_matrix[ch][:, :n_ev]

    E_p = {ch: np.median(cal_matrix[ch], axis=1) for ch in cal_matrix}
    print(f"[wclib] Calibrazione piedistallo PADME (per-capacitor): run {run}, {n_ev} eventi, {len(E_p)} canali.")
    return E_p


def open_mid(run, path='/tmp/',  cloud=True, tag='WC', verbose=False,
             pedestal_correction=True, pedestal_calib_events=PEDESTAL_CALIB_EVENTS):
    import midas.file_reader
    import os
    fname = mid_file(run, tag=tag, cloud=cloud, verbose=verbose)
    if verbose: print(fname)
    if not cloud:
        if os.path.exists(path+tag+fname):
            f = midas.file_reader.MidasFile(path+tag+fname)
        else:
            raise myError("openFileError: "+path+tag+fname+" do not exist")
    else:
        filetmp = cmd.cache_file(fname, cachedir=path, verbose=verbose)
        f = midas.file_reader.MidasFile(filetmp)

    global _current_pedestal
    if pedestal_correction:
        # calibrazione automatica per-run: gli stessi eventi usati per calibrare vengono
        # poi corretti anch'essi nell'uso normale (nessuno split calibrazione/validazione qui,
        # quello serve solo per verificare che la correzione funzioni, vedi SIC_RawView.ipynb)
        _current_pedestal = _build_pedestal_calibration(run, path, cloud, tag, verbose, pedestal_calib_events)
    else:
        _current_pedestal = None

    return f

def get_bor_odb(mfile): # function to acquire the begin of run ODB entries from the midas file
    try:
        odb = mfile.get_bor_odb_dump()
    except:
        myError("No begin-of-run ODB dump found")
    
    mfile.jump_to_start()
    return odb

