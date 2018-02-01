% Image Analysis by Christian Oliveros 2018/01/23
% Last Revision 2018/02/01
% For his master thesis
clearvars; clc;

current_directory = dir();
current_directory(~[current_directory.isdir]) = [];  %remove non-directories
tf = ismember( {current_directory.name}, {'.', '..'});
current_directory(tf) = [];  %remove current and parent directory.

for i = 1:length(current_directory)
    disp(current_directory(i).name)
    analysis_function(current_directory(i).name)
end
    