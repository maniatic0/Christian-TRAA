% Image Analysis by Christian Oliveros 2018/01/23
% Last Revision 2018/02/01
% For his master thesis
clearvars; clc;

current_directory = dir();
current_directory(~[current_directory.isdir]) = [];  %remove non-directories
tf = ismember( {current_directory.name}, {'.', '..'});
current_directory(tf) = [];  %remove current and parent directory.


fprintf('Amount of Tests found: %d\n', length(current_directory));
fprintf('Starting Tests\n\n');

for i = 1:length(current_directory)
	fprintf('Working on test: %s\n', current_directory(i).name);
	fprintf('Return Code of the Tests: %d\n', ...
		analysis_function(current_directory(i).name));
end

fprintf('\nTests Done\n');
    