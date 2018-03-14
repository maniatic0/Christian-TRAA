function [ mse, peaksnr, snr, ssimval, ssimmap, ...
           niqeI, niqeRef, brisqueI, brisqueRef ] ...
           = Test_Files( Test_Image, Reference )
%TEST_FILES Perform MSE, PSNR, SSIM, NIQE and BRISQUE tests to Test_Image
%       mse mean square error
%       peaksnr peak value from PSNR
%       snr value from PSNR
%       ssimval value from SSIM
%       ssimmap image from SSIM
%       niqeI score from NIQUE
%       niqeRef reference score from NIQUE
%       brisqueI score from BRISQUE
%       brisqueRef reference score from BRISQUE

mse = immse(Test_Image, Reference);
[peaksnr, snr] = psnr(Test_Image, Reference);
[ssimval, ssimmap] = ssim(Test_Image, Reference);
try
    niqeI = niqe(Test_Image);
catch
    warning("Niqe failed for Test_Image")
    niqeI = NaN;
end


try
    niqeRef = niqe(Reference);
catch
    warning("Niqe failed for Reference")
    niqeRef = NaN;
end
brisqueI = brisque(Test_Image);
brisqueRef = brisque(Reference);

end

