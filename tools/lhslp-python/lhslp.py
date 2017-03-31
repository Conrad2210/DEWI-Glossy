import copy

class Packet:
    
    MAIN_TYPE_DATA          = 1
    MAIN_TYPE_DATA_REQ      = 2
    MAIN_TYPE_DBG_DATA      = 3
    MAIN_TYPE_DBg_DATA_REQ  = 4
    
    SUB_TYPE_APP_DATA       = 1
    SUB_TYPE_STREAM_REQ     = 2
    SUB_TYPE_STREAM_ACK     = 3
    SUB_TYPE_SCHED          = 4
    SUB_TYPE_DBG_DATA       = 5
    
    
    def __init__(self, main_type, sub_type, data):
        """ Default constructor for LHSLP packet object
        Args:
            main_type:    Main type of the packet
            sub_type:    Sub type of the packet
            data:        Packet data
        """
        self.main_type = main_type
        self.sub_type = sub_type
        self.data = copy.deepcopy(data)
        
    def get_raw_bytes(self):
        """ Get raw bytes of the packet 
        Returns:
            Raw bytes of the packet as a list
        """
        new_data = []
        packet_type = ((self.main_type & 0x0F) << 4) | (self.sub_type & 0x0F)
        new_data.append(packet_type)
        new_data.extend(self.data)
        return new_data

class PacketFactory:
    def create_from_raw_bytes(self, data):
        """ Create a LHSLP packet object from raw data
        Args:
            data: Raw bytes as a list
        Return:
            A LHSLP packet object. None is returned if failed to construct a packet object.
        """
        if data[0] == 0:
            return None
        main_type = (data[0] & 0xF0) >> 4
        sub_type = data[0] & 0x0F
        return Packet(main_type, sub_type, data[1:])
 

class LHSLP:
    
    __HSLP_CHAR_END        = 0xC0
    __HSLP_CHAR_ESC        = 0xDB
    __HSLP_CHAR_ESC_END    = 0xDC
    __HSLP_CHAR_ESC_ESC    = 0xDD
            
    __HSLP_STATE_OK        = 1
    __HSLP_STATE_ESC       = 2
    __HSLP_STATE_RUBBISH   = 3

    def __init__(self):
        self.decode_state = self.__HSLP_STATE_RUBBISH
        self.lst_decoded_data = []
        self.o_packet_factory = PacketFactory()

    def decode_packet(self, lst_characters):
        """ Decode a LHSLP packet from list of characters
            
        This function can be used to decode LHSLP packets. The characters are consumed from index 0 of 
        the lst_characters. Erroneous characters and consumed characters are removed and unconsumed
        characters are left in the lst_characters.
            
        Args: 
            lst_characters:    A character list
        Returns: None if it is not possible to decode the packet. 
        """
        while len(lst_characters) > 0:
            byte = ord(lst_characters[0])
            if self.decode_state == self.__HSLP_STATE_RUBBISH:
                if byte == self.__HSLP_CHAR_END:
                    # we found an start
                    self.decode_state = self.__HSLP_STATE_OK
                    self.lst_decoded_data = []
                lst_characters.pop(0) 
            elif self.decode_state == self.__HSLP_STATE_OK:
                if byte == self.__HSLP_CHAR_END:
                    # we found an end
                    self.decode_state = self.__HSLP_STATE_RUBBISH
                    if len(self.lst_decoded_data) > 0:
                        return self.o_packet_factory.create_from_raw_bytes(self.lst_decoded_data)
                elif byte == self.__HSLP_CHAR_ESC:
                    # we found escape character. So we expect an escaped character
                    self.decode_state = self.__HSLP_STATE_ESC
                    lst_characters.pop(0)
                else:
                    # Some other character
                    self.lst_decoded_data.append(byte)
                    lst_characters.pop(0)
            elif self.decode_state == self.__HSLP_STATE_ESC:
                if byte == self.__HSLP_CHAR_ESC_END:
                    self.decode_state = self.__HSLP_STATE_OK
                    self.lst_decoded_data.append(self.__HSLP_CHAR_END)
                elif byte == self.__HSLP_CHAR_ESC_ESC:
                    self.decode_state = self.__HSLP_STATE_OK
                    self.lst_decoded_data.append(self.__HSLP_CHAR_ESC)
                else:
                    # erroneous character found
                    self.decode_state = self.__HSLP_STATE_RUBBISH
                    self.lst_decoded_data = []
                lst_characters.pop(0)
                
        return None
                    
    def encode_packet(self, packet):
        """ Encode a LHSLP packet object to LHSLP character format 
        Args:
            packet: A LHSLP packet object
        Returns:
            A list of encoded characters
        """
        lst_buffer = []
        lst_buffer.append(self.__HSLP_CHAR_END)
        for byte in packet.get_raw_bytes():
            if byte == self.__HSLP_CHAR_END:
                lst_buffer.append(self.__HSLP_CHAR_ESC)
                lst_buffer.append(self.__HSLP_CHAR_ESC_END)
            elif byte == self.__HSLP_CHAR_ESC:
                lst_buffer.append(self.__HSLP_CHAR_ESC)
                lst_buffer.append(self.__HSLP_CHAR_ESC_ESC)
            else:
                lst_buffer.append(byte)
        lst_buffer.append(self.__HSLP_CHAR_END)
        return lst_buffer
