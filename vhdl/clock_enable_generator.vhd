library ieee;
use ieee.std_logic_1164.all;

entity clock_enable_generator is
	generic ( divider : positive );
	port ( clk : in  std_logic;
		    clk_enable : out  std_logic );
end clock_enable_generator;

architecture behavioral of clock_enable_generator is
	signal cnt : integer range 0 to divider-1;
begin

process(clk)
begin
	if rising_edge(clk) then
		if cnt = divider-1 then
			clk_enable  <= '1';
			cnt <= 0;
		else
			clk_enable  <= '0';
			cnt <= cnt +1;
		end if;
	end if;
end process;

end behavioral;
