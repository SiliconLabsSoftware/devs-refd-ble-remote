import threading
from typing import Optional
from .adpcm import IMAAdpcmDecoder
from .wav import WavWriter

class Audio:
    """High-level audio recorder. Processes ADPCM data and writes WAV files. """

    class Config:

        def __init__(self, sample_rate: int = 16000, channels: int = 1, encoding: bool = True) -> None:
            """Initialize Audio configuration.
            Args:
                sample_rate: Sample rate in Hz (8000 or 16000)
                channels: Number of audio channels (1 or 2)
                encoding: Whether ADPCM decoding is enabled
            """
            self.encoding = encoding
            self.sample_rate = sample_rate
            assert sample_rate == 8000 or sample_rate == 16000, "Sample rate must be 8000 or 16000"
            self.channels = channels
            assert channels == 1 or channels == 2, "Channels must be 1 or 2"

    class Statistics:

        def __init__(self) -> None:
            self.frames_processed = 0
            self.bytes_processed = 0
            self.frames_lost = 0

    class _PacketHeader:

        def __init__(self, data: bytes, channel_count: int = 1) -> None:
            self.index = [data[(4 * i) + 1] for i in range(channel_count)]
            self.predictor = [int.from_bytes(data[(4 * i) + 2:(4 * i) + 4], 'big', signed=True) for i in range(channel_count)]
            self.sequence_counter = data[0]
            self.len = 8

    def __init__(self, config: Config, packet_header_present: bool = True, wav_path: Optional[str] = None) -> None:
        """Initialize Audio class.

        Args:
            config: Configuration for audio processing
            packet_header_present: Whether packet headers are present in audio data
            wav_path: Path to WAV file for writing audio data
        """
        self.config = config
        self.statistics = Audio.Statistics()
        self._is_packet_header_present = packet_header_present
        self._seq_counter = -1
        self._wav_path = wav_path  # Store path for reset() recovery
        self._writer = WavWriter(wav_path, config.sample_rate, channels=config.channels) if wav_path else None
        self._decoder = IMAAdpcmDecoder() if config.encoding else None
        self._lock = threading.RLock()

    def reset(self):
        with self._lock:
            self._seq_counter = -1
            if self._writer:
                self._writer.close()

    def process(self, data: bytes) -> bytes:
        with self._lock:
            self.statistics.frames_processed += 1
            self.statistics.bytes_processed += len(data)

            if self._decoder:
                decoded_data = []
                header = None
                if self._is_packet_header_present:
                    header = Audio._PacketHeader(data, self.config.channels)
                    if self._seq_counter != -1:
                        self.statistics.frames_lost += (self._get_sequence_counter_diff(header.sequence_counter) - 1)
                    else:
                        self._seq_counter = header.sequence_counter
                    data_offs = header.len
                else:
                    data_offs = 0
                for i in range(self.config.channels):
                    if self._is_packet_header_present and header:
                        self._decoder.reset(index=header.index[i], predictor=header.predictor[i])
                    decoded_data.append(self._decoder.decode(data[data_offs + i :: self.config.channels]))
                sample_count = min(len(ch) // 2 for ch in decoded_data)
                data = b''.join(ch[i*2:i*2+2] for i in range(sample_count) for ch in decoded_data)

            if self._writer:
                try:
                    self._writer.write(data)
                except Exception as e:
                    import logging
                    logger = logging.getLogger(__name__)
                    logger.error("Error writing audio data to WAV file: %s", e)
                    if self._wav_path:
                        try:
                            logger.debug("Attempting to recreate WAV writer with path: %s", self._wav_path)
                            self._writer.close()
                            self._writer = WavWriter(self._wav_path, self.config.sample_rate, channels=self.config.channels)
                            self._writer.write(data)
                            logger.debug("Successfully recreated WAV writer and wrote data")
                        except Exception as e2:
                            logger.error("Failed to recreate WAV writer: %s", e2)
                            self._writer = None
            return data

    def get_statistics(self) -> Statistics:
        with self._lock:
            return self.statistics

    def _get_sequence_counter_diff(self, seq_counter: int) -> int:
        diff = seq_counter - self._seq_counter if seq_counter >= self._seq_counter else ((0xFF + seq_counter + 1) - self._seq_counter)
        self._seq_counter = seq_counter
        return diff
