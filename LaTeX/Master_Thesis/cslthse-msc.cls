% cslthse-msc.cls
% Copyright 2013 Flavius Gruian <flavius.gruian@cs.lth.se>
% 
% Licensed under the GNU GPL version 3.0 or later.
% See http://www.gnu.org/licenses/gpl.html
%
\NeedsTeXFormat{LaTeX2e}
\ProvidesClass{cslthse-msc}
		[2015/01/16 v0.5 
		Masters Thesis Class, Department of Computer Science, Lund University]

%\ExecuteOptions{12pt,a4paper,onecolumn, twoside, openright}
%\ProcessOptions\relax
%\LoadClass{report}
\LoadClass[12pt,a4paper,onecolumn,twoside,openright]{report}

% margins
% use showframe in parameters if you want to check the margins
\RequirePackage[left=2.5cm,right=2.5cm,top=3cm,bottom=3cm, bindingoffset=1cm]{geometry}

% Fonts! needs XeTeX
\RequirePackage{ifxetex}
\makeatletter
% for textbullets
\usepackage{textcomp}

   \ifxetex
		\usepackage[no-math]{fontspec}
	 	\setmainfont{TeX Gyre Termes}
	   	\setsansfont{TeX Gyre Heros}
    	\setmonofont{TeX Gyre Cursor}
	\else
		\ClassWarning{cslthse-msc}{Use XeTeX for better fonts!}		
		\usepackage[T1]{fontenc}
		\usepackage{qtxmath}	
		\usepackage{tgtermes}
		\usepackage{tgheros}
		\usepackage{tgcursor}

%		\InputIfFileExists{garamond.sty}{\AtBeginDocument{\garamond}}{%
%			\ClassError{cslthse-msc}{Missing garamond.sty. Unpack the zip from http://gael-varoquaux.info/computers/garamond/index.html in your source directory.}{}}
	
			
	\fi
\makeatother

% fancy headers, footers
\RequirePackage{fancyhdr}
\makeatletter
\pagestyle{fancy}
\renewcommand{\chaptermark}[1]{\markboth{\thechapter.\ #1}{}}
\renewcommand{\sectionmark}[1]{\markright{\thesection\ #1}{}}
\fancyhf{}
\fancyhead[LE]{\footnotesize{\textsc{\leftmark}}}
\fancyhead[RO]{\footnotesize{\textsc{\rightmark}}}
\fancyfoot[LE,RO]{\thepage}
\renewcommand{\headrulewidth}{0.4pt}
\renewcommand{\footrulewidth}{0.4pt}

\fancypagestyle{plain}{
  \fancyhead{}
  \renewcommand{\headrulewidth}{0pt}
%  \renewcommand{\footrulewidth}{0pt}
}
\makeatother

% Title page
\RequirePackage{titling}

\makeatletter
    % custom commands
	\newcommand{\company}[1]{\def \@company {#1}}
	\newcommand{\supervisor}[1]{\def \@supervisor {#1}}
	\newcommand{\supervisors}[2]{\def \@firstsupervisor {#1}  \def \@secondsupervisor {#2}}
	\newcommand{\examiner}[1]{\def \@examiner {#1}}
	\newcommand{\subtitle}[1]{\def \@subtitle {#1}}
	\company{}
	\supervisor{}
	\supervisors{}{}
	\examiner{}
	\subtitle{}
	
	% for the title page
	\pretitle{\noindent\rule{\linewidth}{1pt}\begin{center}\Huge}
	\posttitle{\par \vskip 0.5em \ifx \@subtitle \empty \vskip 1em \else {\Large(\@subtitle)} \fi \end{center}\noindent\rule{\linewidth}{1pt}\vskip 0.5em}
	\predate{\vskip 5em \begin{center}\Large}
	\postdate{\par\vfill {\large Master's thesis work carried out at 
		\ifx\@company\empty
		 	\ClassWarning{cslthse-msc}{Host company missing. Use \protect\company{name}. Defaulting to CS.}
		 	\\ the Department of Computer Science, Lund University.
		\else \@company. \fi }
		  
			\vskip 1em 
			{\normalsize
			\ifx\@supervisor\empty 
			   %\ClassWarning{cslthse-msc}{Empty supervisor.}
			   \ifx\@firstsupervisor\empty
				  \ClassError{cslthse-msc}{Missing supervisor. Use either \protect\supervisor{name,email} or \protect\supervisors{name1,email1}{name2,email2}}{You must specify your thesis supervisor(s).}
				\else Supervisors: \@firstsupervisor ~\\ \@secondsupervisor \fi
			\else Supervisor: \@supervisor \fi
			
			Examiner: 
			\ifx\@examiner \empty
				\ClassError{cslthse-msc}{Missing examiner. Use \protect\examiner{name,email}}{You must specify your thesis examiner.}
			\else	 \@examiner \fi } \end{center}}

\makeatother


% Sections and captions
\RequirePackage{titlesec}
\makeatletter
	\titleformat{\chapter}[display]{\normalfont\huge\bfseries}
	{\chaptertitlename\ \thechapter}{10pt}{\Huge}[\vspace{2cm}\hrulefill]
	%\titleformat{\section}[hang]{\LARGE\bfseries\sffamily}{\thesection}{}
	\titleformat*{\section}{\LARGE\bfseries\sffamily}
	\titleformat*{\subsection}{\Large\bfseries\sffamily}
	\titleformat*{\subsubsection}{\large\bfseries\sffamily}
	\titleformat*{\paragraph}{\large\bfseries\sffamily}
	\titleformat*{\subparagraph}{\large\bfseries\sffamily}
\makeatother


% captions for figures and tables
\RequirePackage{caption}
\makeatletter
	\renewcommand{\captionlabelfont}{\bfseries}
	\setlength{\captionmargin}{2cm}
\makeatother

% takes care of the abstract and aknowledgements
\RequirePackage{abstract}

\makeatletter

\newcommand{\keywords}[1]{\def \@keywords {#1}}
\newcommand{\theabstract}[1]{\def \@theabstract {#1}}
\newcommand{\acknowledgements}[1]{\def \@acknowledgements {#1}}

\keywords{}
\theabstract{}
\acknowledgements{}

% indent the abstract properly
\setlength{\absrightindent}{1cm}
\setlength{\absleftindent}{1cm}

% makes everything in the begining of the document
\newcommand{\makefrontmatter}{
	\maketitle

	% make the abstract and keywords
	\ifx \@theabstract \@empty
		\ClassWarning{cslthse-msc}{The abstract is missing. Use \protect\theabstract{text}.}
	\else
		% empty page		
		\pagestyle{empty}
		% make sure the abstract is on a right-hand side
		\cleardoublepage
		\begin{abstract}
		\noindent
		\@theabstract		
		\ifx \@keywords \@empty
			\ClassWarning{cslthse-msc}{Keywords are missing. Use \protect\keywords{words}.}
		\else
			\vspace{2cm}\par\noindent {\small{\bf Keywords\/}: \@keywords}
		\fi
		\end{abstract}
	\fi
	
	
	\pagestyle{fancy}

	% add acknowledgement if any
	\ifx \@acknowledgements \@empty
			\relax
	\else
			\chapter*{Acknowledgements}
			\@acknowledgements
	\fi

	\tableofcontents
}	
\makeatother


% make the bibliography
\makeatletter
\newcommand{\makebibliography}[1]{
	\cleardoublepage
	\phantomsection
	\addcontentsline{toc}{chapter}{Bibliography}
	\bibliographystyle{plain}
	\bibliography{#1}
}