export const DEFAULT_BUFFER_LEN = 128;
export const MAX_CHANNELS = 32;
export const RING_BUFFER_SIZE = 16384;
export const MIDI_BUFFER_SIZE = 1024;
export const MIDI_BUFFER_PAYLOAD_SIZE = 3;
export const CALLBACK_DATA_BUFFER_SIZE = 16384;

export const initialSharedState = [
  0, // 1 = csound worker is locked
  4096, // n = framesRequested (4096 is only initial and will thereafter be max'd to 2048)
  0, // 1 = Csound is currently performing
  0, // 1 = Csound is currently paused
  0, // 1 = STOP
  0, // n = sample rate
  0, // n = ksmps
  0, // n = nchnls
  0, // n = ncnls_i
  0, // n = if 1 then it's requesting microphone
  DEFAULT_BUFFER_LEN, // number of samples per process
  0, // n = buffer read index of output buffer
  0, // n = buffer write index of output buffer
  0, // n = number of input buffers available
  0, // n = number of output buffers available
  0, // n = if 1 then is requesting rtmidi
  0, // n = rtmidi buffer index
  0, // n = available rtmidi events in buffer
  0, // n = has pending callbacks
];

// Enum helper for SAB
export const AUDIO_STATE = {
  CSOUND_LOCK: 0,
  FRAMES_REQUESTED: 1,
  IS_PERFORMING: 2,
  IS_PAUSED: 3,
  STOP: 4,
  SAMPLE_RATE: 5,
  KSMPS: 6,
  NCHNLS: 7,
  NCHNLS_I: 8,
  IS_REQUESTING_MIC: 9,
  BUFFER_LEN: 10,
  OUTPUT_READ_INDEX: 11,
  OUTPUT_WRITE_INDEX: 12,
  AVAIL_IN_BUFS: 13,
  AVAIL_OUT_BUFS: 14,
  IS_REQUESTING_RTMIDI: 15,
  RTMIDI_INDEX: 16,
  AVAIL_RTMIDI_EVENTS: 17,
  HAS_PENDING_CALLBACKS: 18,
};

export const DATA_TYPE = {
  NUMBER: 0,
  STRING: 1,
  FLOAT_32: 2,
  FLOAT_64: 3,
};
