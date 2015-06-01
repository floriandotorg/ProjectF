library ieee;
use ieee.std_logic_1164.all;
 
entity toplevel_test is
end toplevel_test;
 
architecture behavior of toplevel_test is 
 
    component toplevel
    port( clk : in std_logic;
		    sseg : out  std_logic_vector (7 downto 0);
		    anodes : out  std_logic_vector (3 downto 0);
		    leds : out std_logic_vector (7 downto 0);
		    switches : in std_logic_vector (7 downto 0);
			 mem_oe : out std_logic;
			 mem_we : out std_logic;
			 ram_adv : out std_logic;
			 ram_ce : out std_logic;
			 ram_clk : out std_logic;
			 ram_cre : out std_logic;
			 ram_lb : out std_logic;
			 ram_ub : out std_logic;
			 ram_wait : in std_logic;
			 flash_rp : out std_logic;
			 flash_ce : out std_logic;
			 --flash_st_sts : in std_logic;
			 mem_addr : out std_logic_vector (23 downto 1);
			 mem_data : inout std_logic_vector (15 downto 0)
        );
    end component;
    
   --inputs
   signal clk : std_logic := '0';
	signal ram_wait : std_logic := '0'; 
	signal switches : std_logic_vector (7 downto 0) := (others => '0'); 

 	--outputs
   signal sseg : std_logic_vector(7 downto 0);
   signal anodes : std_logic_vector(3 downto 0);
   signal leds : std_logic_vector(7 downto 0);
	signal mem_oe : std_logic;
	signal mem_we : std_logic;
	signal ram_adv : std_logic;
	signal ram_ce : std_logic;
	signal ram_clk : std_logic;
	signal ram_cre : std_logic;
	signal ram_lb : std_logic;
	signal ram_ub : std_logic;
	signal flash_rp : std_logic;
	signal flash_ce : std_logic;
	signal mem_addr : std_logic_vector (23 downto 1);
	
	signal mem_data : std_logic_vector (15 downto 0);

   -- clock period definitions
   constant clk_period : time := 10 ps;
 
begin
 
	-- instantiate the unit under test (uut)
   uut: toplevel port map (
          clk => clk,
          sseg => sseg,
          anodes => anodes,
          leds => leds,
			 switches => switches,
			 mem_oe => mem_oe,
			 mem_we => mem_we,
			 ram_adv => ram_adv,
			 ram_ce => ram_ce,
			 ram_clk => ram_clk,
			 ram_cre => ram_cre,
			 ram_lb => ram_lb,
			 ram_ub => ram_ub,
			 ram_wait => ram_wait,
			 flash_rp => flash_rp,
			 flash_ce => flash_ce,
			 mem_addr => mem_addr,
			 mem_data => mem_data
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
