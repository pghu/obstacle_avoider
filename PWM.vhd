------------------------------------------------------------------------------
--Original Author: Robert Hood
--Original Group: Group 1
--Original Project: iOS Remote Control Car
--Original Date: Feb. 27, 2011

--Author: Peter Hu
--Group: Group 14
--Project: Autonomous Rover
--Date: March 9, 2013
------------------------------------------------------------------------------
library altera;
use altera.altera_europa_support_lib.all;

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_arith.all;
use ieee.std_logic_unsigned.all;

entity pwm is

	port(
		csi_myclock_clk			:	in		std_logic;
		csi_myclock_reset			:	in		std_logic;

		avs_pwm_write_n			:	in		std_logic;
		avs_pwm_chipselect		:	in		std_logic;
		avs_pwm_address			:	in		std_logic_vector(1 downto 0);
		avs_pwm_readdata			:	out	std_logic_vector(15 downto 0);
		avs_pwm_writedata			:	in		std_logic_vector(15 downto 0);

		coe_pwm_output_export	:	out	std_logic
	);

end entity;

architecture pwm_control of pwm is

	signal pwm_signal				:	std_logic_vector(19 downto 0); --Represents the duty cycle
	signal counter					:	std_logic_vector(19 downto 0) := (others => '0'); --20 bit counter

begin

	process(csi_myclock_clk,csi_myclock_reset)
	begin

		if csi_myclock_reset= '1' then
			pwm_signal <= (others => '0');

		elsif csi_myclock_clk'event and csi_myclock_clk = '1' then
			if std_logic'(((avs_pwm_chipselect AND NOT avs_pwm_write_n) AND to_std_logic((((std_logic_vector'("000000000000000000000000000000") & (avs_pwm_address)) = std_logic_vector'("00000000000000000000000000000000")))))) = '1' then
				pwm_signal <= "0000" & avs_pwm_writedata(15 DOWNTO 0);
			end if;
		end if;
	end process;

	process(csi_myclock_clk)
	begin

		if csi_myclock_reset='1' then
			counter <= (others => '0');
		elsif rising_edge(csi_myclock_clk) then
			counter <= counter +1;
		end if;

		if(counter = "0000111101000010010") then
			counter <= (others => '0');
		end if;
	
		if ((pwm_signal>counter)and (pwm_signal>0)) then
			coe_pwm_output_export<='1';
		else
			coe_pwm_output_export<='0';
		end if;

	end process;
end pwm_control;
