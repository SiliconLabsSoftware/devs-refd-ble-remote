from dataclasses import dataclass
from typing import Optional
import struct

_IMA_INDEX_TABLE = (
  -1, -1, -1, -1, 2, 4, 6, 8,
  -1, -1, -1, -1, 2, 4, 6, 8,
)

_IMA_STEP_TABLE = (
  7, 8, 9, 10, 11, 12, 13, 14, 16, 17,
  19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
  50, 55, 60, 66, 73, 80, 88, 97, 107, 118,
  130, 143, 157, 173, 190, 209, 230, 253, 279, 307,
  337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
  876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066,
  2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358,
  5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
  15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767,
)

@dataclass
class IMAState:
    predictor: int = 0
    index: int = 0

class IMAAdpcmDecoder:
    """Decoder for IMA ADPCM encoded audio data. Supports nibble and block modes."""

    def __init__(self, high_nibble_first: bool = False):
        """Initialize IMA ADPCM decoder. """
        self._high_first = high_nibble_first
        self._state = IMAState()

    @property
    def state(self) -> IMAState:
        return self._state

    def reset(self, predictor: int = 0, index: int = 0) -> None:
        """Reset decoder state to specified predictor and index values. """
        self._state.predictor = int(predictor)
        self._state.index = int(index)

    def decode(self, data: bytes, state: Optional[IMAState] = None) -> bytes:
        return self._decode_bytes(data, state or self._state)

    def _decode_bytes(self, data: bytes, state: IMAState) -> bytes:
        out = bytearray()
        pack = struct.pack
        for b in data:
            if self._high_first:
                codes = ((b >> 4) & 0x0F, b & 0x0F)
            else:
                codes = (b & 0x0F, (b >> 4) & 0x0F)
            for c in codes:
                sample = self._decode_nibble(c, state)
                out += pack('<h', sample)
        return bytes(out)

    def _decode_nibble(self, code: int, state: IMAState) -> int:
        if state.index < 0:
            state.index = 0
        elif state.index > 88:
            state.index = 88
        step = _IMA_STEP_TABLE[state.index]

        diff = step >> 3
        if code & 1:
            diff += step >> 2
        if code & 2:
            diff += step >> 1
        if code & 4:
            diff += step

        if code & 8:
            state.predictor -= diff
        else:
            state.predictor += diff

        if state.predictor < -32768:
            state.predictor = -32768
        elif state.predictor > 32767:
            state.predictor = 32767

        state.index += _IMA_INDEX_TABLE[code & 0x0F]
        if state.index < 0:
            state.index = 0
        elif state.index > 88:
            state.index = 88

        return state.predictor
