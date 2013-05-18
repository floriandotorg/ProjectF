library ieee;
use ieee.std_logic_1164.all;

package sseg is
	subtype sseg_value is std_logic_vector (3 downto 0);
	type sseg_value_arr is array (0 to 3) of sseg_value;
end sseg;

library ieee;
use ieee.std_logic_1164.all;
use work.sseg.all;

entity sseg_controller is
   port ( clk : in  std_logic;
          clk_enable : in  std_logic;
			 values : in sseg_value_arr;
			 sseg : out  std_logic_vector (7 downto 0);
			 anodes : out  std_logic_vector (3 downto 0));
end sseg_controller;

architecture behavioral of sseg_controller is
	signal anode_state : std_logic_vector (3 downto 0);
	signal uncoded_sseg : sseg_value;
begin

anodes <= anode_state;

with uncoded_sseg select
--          PGFEDCBA
	sseg <= "11000000" when "0000", -- '0'
		     "11111001" when "0001", -- '1'
			  "10100100" when "0010", -- '2'
			  "10110000" when "0011", -- '3'
			  "10011001" when "0100", -- '4'
			  "10010010" when "0101", -- '5'
			  "10000010" when "0110", -- '6'
			  "11111000" when "0111", -- '7'
			  "10000000" when "1000", -- '8'
			  "10010000" when "1001", -- '9'
			  "10001000" when "1010", -- 'A'
			  "10000011" when "1011", -- 'b'
			  "10100111" when "1100", -- 'c'
			  "10100001" when "1101", -- 'd'
			  "10000110" when "1110", -- 'E'
			  "10001110" when "1111", -- 'F';
			  "11111111" when others;

process(clk)
begin
  if rising_edge(clk) then
    if clk_enable='1' then
			case anode_state is
				when "0111" =>
					anode_state <= "1011";
					uncoded_sseg <= values(1);
				when "1011" =>
					anode_state <= "1101";
					uncoded_sseg <= values(2);
				when "1101" =>
					anode_state <= "1110";
					uncoded_sseg <= values(3);
				when others =>
					anode_state <= "0111";
					uncoded_sseg <= values(0);
			end case;
    end if;
  end if;
end process;

end behavioral;
