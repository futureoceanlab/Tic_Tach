function orig_val = recover_mist(old_val)
% 
% B1 = bitand(bitshift(old_val, -28), 15);
% C2 = bitand(bitshift(old_val, -24), 15);
% C1 = bitand(bitshift(old_val, -16), 15);
% D2 = bitand(bitshift(old_val, -8), 15);
% D1 = bitand(old_val, 15);
% orig_val = B1*2^16+C2*2^12+C1*2^8+D2*2^4+D1;

B1 = bitand(bitshift(old_val, -28), 15);
C2 = bitand(bitshift(old_val, -20), 15);
C1 = bitand(bitshift(old_val, -12), 15);
D = bitand(old_val, 255);
orig_val = B1*2^16+C2*2^12+C1*2^8+D;

end