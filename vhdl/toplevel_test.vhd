library ieee;
use ieee.std_logic_1164.all;
 
entity toplevel_test is
end toplevel_test;
 
architecture behavior of toplevel_test is 
 
    component toplevel
    port(
         clk : in  std_logic;
         sseg : out  std_logic_vector(7 downto 0);
         anodes : out  std_logic_vector(3 downto 0);
         leds : out  std_logic_vector(7 downto 0)
        );
    end component;
    
   --inputs
   signal clk : std_logic := '0';

 	--outputs
   signal sseg : std_logic_vector(7 downto 0);
   signal anodes : std_logic_vector(3 downto 0);
   signal leds : std_logic_vector(7 downto 0);

   -- clock period definitions
   constant clk_period : time := 10 ps;
 
begin
 
	-- instantiate the unit under test (uut)
   uut: toplevel port map (
          clk => clk,
          sseg => sseg,
          anodes => anodes,
          leds => leds
        );

   -- clock process definitions
   clk_process :process
   begin
		clk <= '0';
		wait for clk_period/2;
		clk <= '1';
		wait for clk_period/2;
   end process;
 

   -- stimulus process
   stim_proc: process
   begin		
      -- hold reset state for 100 ns.
      wait for 100 ns;	

      wait for clk_period*10;

      -- insert stimulus here 

      wait;
   end process;

end;
