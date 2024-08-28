use std::{
    fs,
    io::{self, Write},
    path::Path,
    str,
    time::Instant,
};

use anyhow::{anyhow, Result};
use tree_sitter_loader::{Config, Loader};
use tree_sitter_tags::TagsContext;

use super::util;

pub fn generate_tags(
    loader: &Loader,
    loader_config: &Config,
    scope: Option<&str>,
    paths: &[String],
    quiet: bool,
    time: bool,
) -> Result<()> {
    let mut lang = None;
    if let Some(scope) = scope {
        lang = loader.language_configuration_for_scope(scope)?;
        if lang.is_none() {
            return Err(anyhow!("Unknown scope '{scope}'"));
        }
    }

    let mut context = TagsContext::new();
    let cancellation_flag = util::cancel_on_signal();
    let stdout = io::stdout();
    let mut stdout = stdout.lock();

    for path in paths {
        let path = Path::new(&path);
        let (language, language_config) = match lang.clone() {
            Some(v) => v,
            None => {
                if let Some(v) = loader.language_configuration_for_file_name(path)? {
                    v
                } else {
                    eprintln!("{}", util::lang_not_found_for_path(path, loader_config));
                    continue;
                }
            }
        };

        if let Some(tags_config) = language_config.tags_config(language)? {
            let indent = if paths.len() > 1 {
                if !quiet {
                    writeln!(&mut stdout, "{}", path.to_string_lossy())?;
                }
                "\t"
            } else {
                ""
            };

            let source = fs::read(path)?;
            let t0 = Instant::now();
            for tag in context
                .generate_tags(tags_config, &source, Some(&cancellation_flag))?
                .0
            {
                let tag = tag?;
                if !quiet {
                    write!(
                        &mut stdout,
                        "{indent}{:<10}\t | {:<8}\t{} {} - {} `{}`",
                        str::from_utf8(&source[tag.name_range]).unwrap_or(""),
                        &tags_config.syntax_type_name(tag.syntax_type_id),
                        if tag.is_definition { "def" } else { "ref" },
                        tag.span.start,
                        tag.span.end,
                        str::from_utf8(&source[tag.line_range]).unwrap_or(""),
                    )?;
                    if let Some(docs) = tag.docs {
                        if docs.len() > 120 {
                            write!(&mut stdout, "\t{:?}...", docs.get(0..120).unwrap_or(""))?;
                        } else {
                            write!(&mut stdout, "\t{:?}", &docs)?;
                        }
                    }
                    writeln!(&mut stdout)?;
                }
            }

            if time {
                writeln!(&mut stdout, "{indent}time: {}ms", t0.elapsed().as_millis(),)?;
            }
        } else {
            eprintln!("No tags config found for path {path:?}");
        }
    }

    Ok(())
}
