#!/usr/bin/perl
use warnings;
use strict;

sub smd
{
	my ($name,$w,$h,$x,$y) = @_;
	printf "Smd %.4f %.4f '%s' (%.4f %.4f);\n",
		$w,
		$h,
		$name,
		$x,
		$y,
		;
}

sub wire
{
	my ($x1,$y1,$x2,$y2) = @_;
	my $w = 0.005;
	#$x1 = $x1/25.4; # + $w/2;
	#$y1 = $y1/25.4; # - $w/2;
	#$x2 = $x2/25.4; # + $w/2;
	#$y2 = $y2/25.4; # - $w/2;
	printf "WIRE %.4f (%.4f %.4f) (%.4f %.4f);\n",
		$w,
		$x1, $y1,
		$x2, $y2,
		;
}


print "GRID MM;\n";

# four side mounting pads
my $y0 = 1.5;
my $w = (11.8 - 7.9) / 2;
my $x = (11.8 - $w)/2;
smd('G0', $w, 2.2, -$x, $y0+0);
smd('G1', $w, 1.8, -$x, $y0+2.4);
smd('G2', $w, 2.2, +$x, $y0+0);
smd('G3', $w, 1.8, +$x, $y0+2.4);

# two top mounting pads
my $y = $y0 + 4.75 - 1.7/2;
smd('G4', 1.2, 1.7,  -4.7/2, $y);
smd('G5', 1.2, 1.7,  +4.7/2, $y);

# And now the five data pins
smd('VCC', 0.4, 1.7,  -1.30, $y);
smd('D-',  0.4, 1.7,  -0.65, $y);
smd('D+',  0.4, 1.7,      0, $y);
smd('ID',  0.4, 1.7,  +0.65, $y);
smd('GND', 0.4, 1.7,  +1.30, $y);


# and the cutout region
my $x1 = (11.8/2);
my $x2 = ( 7.7/2);
my $y1 = 0;
my $y2 = $y1 + (4.45);

print "LAYER 20;\n";

wire(-$x1,$y1, -$x2,$y1);
wire(-$x2,$y1, -$x2,$y2);
wire(-$x2,$y2, +$x2,$y2);
wire(+$x2,$y2, +$x2,$y1);
wire(+$x2,$y1, +$x1,$y1);

__END__

