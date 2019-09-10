% x = 2^18+2^12+17;
% new_val = bitand(x, 15);
% for i=2:4
%    new_val = new_val+bitand(bitshift(x, -4*(i-1)), 255)*(256^(i-1));  
% end
filename = 'candump-2019-09-05_184932.log';
% delimterIn = '#';
% headerlinesIn = 1;
A0 = importdata(filename);
A0 = split(A0, '#');
A = erase(A0, '00000000');
B = hex2dec(A(:, 2));
C = recover_mist(B);

start_idx = 108;
end_idx = 377;
D = C(start_idx:end_idx);
x = (1:(end_idx-start_idx+1))';
b = x\D;
y = x.*b;
std(D - y)


P2 = abs(fD);
P1 = P2(1:(end_idx-start_idx+1)/2+1);
P1(2:end-1) = 2*P1(2:end-1);

